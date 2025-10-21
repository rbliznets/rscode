/*!
    \file
    \brief Class for implementing Reed-Solomon encoding.
    \authors Bliznets R.A. (r.bliznets@gmail.com)
    \version 1.0.1.0
    \date 21.04.2021
*/

#include "RSEncode16.h"
#include <cstring>
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_log.h"

#ifdef CONFIG_RS_IN_RAM
void IRAM_ATTR RSEncode16::poly_remainder(uint8_t *data, uint8_t *data_mod, uint32_t size)
#else
void RSEncode16::poly_remainder(uint8_t *data, uint8_t *data_mod, uint32_t size)
#endif
{
    // Assert that input pointers are valid and size is greater than 16
    assert(data != nullptr);
    assert(data_mod != nullptr);
    assert(size > 16);

    // Calculate the polynomial remainder using the generator polynomial m_G
    // This is essentially polynomial division in GF(2^8) to find the syndrome polynomial coefficients
    uint8_t x = data[0]; // Leading coefficient for the first iteration
    for (uint32_t j = 0; j < 16; j++)
    {
        // Multiply the leading coefficient 'x' with the generator polynomial coefficient m_G[j]
        // and XOR the result with the corresponding data coefficient
        data_mod[j] = data[j + 1] ^ gmul[x][m_G[j]];
    }

    // Continue the polynomial division process for the remaining coefficients
    for (uint32_t i = 1; i < size; i++)
    {
        x = data_mod[0]; // Leading coefficient of the current remainder
        // Shift the remainder coefficients left and update them
        for (uint32_t j = 0; j < 15; j++)
        {
            data_mod[j] = data_mod[j + 1] ^ gmul[x][m_G[j]];
        }
        // Handle the last coefficient separately
        data_mod[15] = data[i + 16] ^ gmul[x][m_G[15]];
    }
}

#ifdef CONFIG_RS_IN_RAM
void IRAM_ATTR RSEncode16::encode(uint8_t *data_in, uint32_t size, uint8_t *data_out)
#else
void RSEncode16::encode(uint8_t *data_in, uint32_t size, uint8_t *data_out)
#endif
{
    // Assert that input pointers are valid and size is positive
    assert(data_in != nullptr);
    assert(data_out != nullptr);
    assert(size > 0);

    // Copy the input data to the output buffer if they are different
    if (data_in != data_out)
        std::memcpy(data_out, data_in, size);

    // Initialize the parity bytes (16 bytes) to zero
    std::memset(&data_out[size], 0, 16);

    // Temporary buffer to store the remainder polynomial coefficients
    uint8_t tmp[16];
    // Calculate the polynomial remainder (syndrome polynomial coefficients) for the message + zero parity
    poly_remainder(data_out, tmp, size);
    // Copy the calculated remainder (the parity bytes) into the output buffer after the message data
    std::memcpy(&data_out[size], tmp, 16);
}

#ifdef CONFIG_RS_IN_RAM
uint8_t IRAM_ATTR RSEncode16::poly_eval(uint8_t alfa, uint8_t *data_poly, uint32_t size)
#else
uint8_t RSEncode16::poly_eval(uint8_t alfa, uint8_t *data_poly, uint32_t size)
#endif
{
    // Assert that the polynomial pointer is valid and size is positive
    assert(data_poly != nullptr);
    assert(size > 0);

    // Evaluate the polynomial at the point galfa[alfa] using Horner's method
    uint8_t x = galfa[alfa];         // Get the Galois field element corresponding to 'alfa'
    uint8_t x1 = x;                  // Powers of 'x' for the evaluation
    uint8_t s = data_poly[size - 1]; // Start with the coefficient of the highest degree term
    for (uint32_t i = 0; i < (size - 1); i++)
    {
        // Horner's method: s = s * x + coefficient_of_next_lower_degree
        s ^= gmul[data_poly[size - 2 - i]][x1]; // Multiply coefficient by current power of x and XOR to result
        x1 = gmul[x1][x];                       // Calculate the next power of x
    }
    return s; // Return the result of the polynomial evaluation
}

#ifdef CONFIG_RS_IN_RAM
uint8_t IRAM_ATTR RSEncode16::poly_eval2(uint8_t data, uint8_t *data_poly, uint32_t size)
#else
uint8_t RSEncode16::poly_eval2(uint8_t data, uint8_t *data_poly, uint32_t size)
#endif
{
    // Assert that the polynomial pointer is valid and size is positive
    assert(data_poly != nullptr);
    assert(size > 0);

    // Evaluate the polynomial at the point 'data' using a slightly different Horner's method implementation
    uint8_t s = data_poly[size - 1]; // Start with the coefficient of the highest degree term
    for (uint32_t i = 0; i < (size - 1); i++)
    {
        // Horner's method: s = s * data + coefficient_of_next_lower_degree
        s = gmul[s][data] ^ data_poly[size - 2 - i]; // Multiply current result by 'data', then XOR with next coefficient
    }
    // The final step multiplies the result by 'data' again (as per the specific polynomial evaluation needed)
    return gmul[s][data];
}

#ifdef CONFIG_RS_IN_RAM
void IRAM_ATTR RSEncode16::decode(uint8_t *data_in, uint8_t *data_out, uint32_t size)
#else
void RSEncode16::decode(uint8_t *data_in, uint8_t *data_out, uint32_t size)
#endif
{
    // Assert that input pointers are valid and size is positive
    assert(data_in != nullptr);
    assert(data_out != nullptr);
    assert(size > 0);

    // Copy the input data (potentially corrupted) to the output buffer if they are different
    if (data_in != data_out)
        std::memcpy(data_out, data_in, size);

    // Calculate the polynomial remainder (syndrome polynomial coefficients) for the received data
    uint8_t tmp[16];
    poly_remainder(data_in, tmp, size);

    // Calculate syndromes S[i] = syndrome_poly(galfa[i+1]) for i = 0 to 15
    uint8_t s[16];
    uint8_t flag = 0; // Flag to indicate if any syndrome is non-zero (errors detected)
    for (uint8_t i = 0; i < 16; i++)
    {
        s[i] = poly_eval(i + 1, tmp, 16); // Evaluate syndrome polynomial at galfa[i+1]
        if (s[i] != 0)
            flag = 1; // Set flag if any syndrome is non-zero
    }
    if (flag == 0) // If no errors detected (all syndromes are zero)
        return;    // Data is correct, no correction needed

    // Berlekamp-Massey algorithm to find the error locator polynomial
    uint8_t err_loc[9] = {1, 0, 0, 0, 0, 0, 0, 0, 0}; // Initialize error locator polynomial (starts as 1)
    uint8_t sz_err = 1;                               // Current degree of the error locator polynomial (+1 for array indexing)
    uint8_t old_loc[9] = {1, 0, 0, 0, 0, 0, 0, 0, 0}; // Store previous version of err_loc
    uint8_t sz_old = 1;                               // Degree of old_loc (+1)
    uint8_t new_loc[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // Temporary storage for calculations
    uint8_t sz_new;                                   // Degree of new_loc (+1)
    for (uint8_t i = 0; i < 16; i++)                  // Process each syndrome
    {
        // Calculate the discrepancy delta for the current iteration
        uint8_t delta = s[i];                // Start with the current syndrome value
        for (uint8_t j = 1; j < sz_err; j++) // Sum contributions from previous error locator coefficients
        {
            delta ^= gmul[err_loc[sz_err - j - 1]][s[i - j]]; // Multiply and XOR
        }
        sz_old++; // Increment degree of old_loc for this iteration

        if (delta != 0) // If discrepancy is non-zero
        {
            if (sz_old > sz_err) // If the current discrepancy indicates a higher degree is needed
            {
                // Update old_loc to the current err_loc (scaled by ginv[delta])
                for (uint8_t j = 0; j < sz_old; j++)
                {
                    new_loc[j] = gmul[delta][old_loc[j]]; // Scale old_loc by delta
                }
                sz_new = sz_old;

                for (uint8_t j = 0; j < sz_err; j++)
                {
                    old_loc[j] = gmul[ginv[delta]][err_loc[j]]; // Scale err_loc by ginv[delta] and store in old_loc
                }
                sz_old = sz_err; // Update sz_old

                // Update err_loc to the new polynomial (scaled old_loc)
                for (uint8_t j = 0; j < sz_new; j++)
                {
                    err_loc[j] = new_loc[j]; // Copy scaled old_loc to err_loc
                }
                sz_err = sz_new; // Update sz_err
            }

            // Update the error locator polynomial using the discrepancy
            uint8_t k = sz_err - sz_old;         // Difference in degrees
            for (uint8_t j = k; j < sz_err; j++) // Add scaled old_loc to err_loc
            {
                err_loc[j] ^= gmul[delta][old_loc[j - k]]; // Multiply old_loc coefficient by delta and XOR
            }
        }
    }

    // Reverse the error locator polynomial coefficients (roots are at reciprocal points)
    for (uint8_t j = 0; j < sz_err; j++)
    {
        new_loc[j] = err_loc[sz_err - 1 - j]; // Reverse the order
    }

    // Find the roots of the error locator polynomial (error positions)
    sz_old = 0;                               // Counter for found error positions
    for (uint8_t j = 0; j < (size + 16); j++) // Check possible error positions (message + parity length)
    {
        // Evaluate the reversed error locator polynomial at galfa[j]
        if (poly_eval(j, new_loc, sz_err) == 0) // If evaluation is zero, it's a root
        {
            old_loc[sz_old] = j; // Store the exponent 'j' (GF element index)
            // Calculate the corresponding position in the original received data
            // (size + 15 - j) because the polynomial was set up for the reversed coefficients
            err_loc[sz_old] = size + 15 - j;
            sz_old++;                   // Increment found error count
            if (sz_old == (sz_err - 1)) // Stop when all expected roots are found
                break;
        }
    }

    // Calculate the error evaluator polynomial Omega(x) = [ Syndromes * Error_locator ] mod x^(n-k+1)
    uint8_t omega[8];         // Store the result (Omega polynomial coefficients)
    std::memset(omega, 0, 8); // Initialize to zero
    // Multiply syndrome polynomial 's' with error locator polynomial 'new_loc', store result in 'omega'
    // The result size is limited to sz_err - 1 (degree of Omega is less than degree of Lambda)
    poly_mul(s, 16, new_loc, sz_err, omega, sz_err - 1);

    // Convert stored exponents (galfa indices) back to their corresponding Galois field values
    for (uint8_t j = 0; j < sz_err - 1; j++)
    {
        old_loc[j] = galfa[old_loc[j]]; // Get the GF(2^8) element value
    }

    // Calculate error values and correct the data
    for (uint8_t i = 0; i < sz_err - 1; i++) // Iterate through each found error
    {
        // Check if the error location is within the original message data (not parity)
        if (err_loc[i] < size)
        {
            // Get the inverse of the Galois field element corresponding to the error location
            uint8_t x_inv = ginv[old_loc[i]];

            // Calculate the derivative of the error locator polynomial at the error location
            // (needed for Forney's algorithm)
            uint8_t err_loc_prime = 1; // Start with 1
            for (uint8_t j = 0; j < sz_err - 1; j++)
            {
                if (i != j) // For each root except the current one
                {
                    // Calculate (1 - alpha^i * alpha^j) where alpha^i is old_loc[i] and alpha^j is old_loc[j]
                    uint8_t x = 1 ^ gmul[x_inv][old_loc[j]]; // 1 + x_inv * root_j (in GF, + is XOR)
                    err_loc_prime = gmul[err_loc_prime][x];  // Multiply all factors together
                }
            }
            // Optional debug check commented out
            //    if(err_loc_prime == 0)
            //    {
            //         ESP_LOGW("RSEncode16","err_loc_prime == 0");
            //    }

            // Calculate the numerator for Forney's algorithm: Omega(x_inv)
            uint8_t y = poly_eval2(x_inv, omega, sz_err - 1);
            // Calculate the multiplicative inverse of the denominator (err_loc_prime)
            err_loc_prime = ginv[err_loc_prime];
            // Apply the error correction: data ^= (numerator / denominator)
            data_out[err_loc[i]] ^= gmul[y][err_loc_prime]; // XOR the correction value
        }
    }
}

#ifdef CONFIG_RS_IN_RAM
void IRAM_ATTR RSEncode16::poly_mul(uint8_t *p1, uint32_t p1_size, uint8_t *p2, uint32_t p2_size, uint8_t *result, uint32_t result_size)
#else
void RSEncode16::poly_mul(uint8_t *p1, uint32_t p1_size, uint8_t *p2, uint32_t p2_size, uint8_t *result, uint32_t result_size)
#endif
{
    // Assert that input pointers and sizes are valid
    assert(p1 != nullptr);
    assert(p1_size > 0);
    assert(p2 != nullptr);
    assert(p2_size > 0);
    assert(result != nullptr);
    assert(result_size > 0);

    // Initialize the result polynomial coefficients to zero
    std::memset(result, 0, result_size);

    // Perform polynomial multiplication in GF(2^8)
    // The result polynomial's degree is limited by result_size
    for (uint32_t n1 = 0; (n1 < result_size) && (n1 < p1_size); n1++) // Iterate through coefficients of p1
    {
        for (uint32_t n2 = 0; n2 < p2_size; n2++) // Iterate through coefficients of p2
        {
            if ((n1 + n2) >= result_size) // Stop if the resulting degree exceeds the result buffer size
                break;
            // Multiply coefficients p1[n1] and p2[n2] using Galois field multiplication
            // and XOR the result into the coefficient of the resulting polynomial at degree (n1+n2)
            result[n1 + n2] ^= gmul[p1[n1]][p2[n2]];
        }
    }
}