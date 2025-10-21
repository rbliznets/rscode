/*!
    \file
    \brief Class for implementing Reed-Solomon encoding.
    \authors Bliznets R.A. (r.bliznets@gmail.com)
    \version 1.0.1.0
    \date 21.04.2021
*/

#pragma once

#include "sdkconfig.h"
#include <stdint.h>

/// Reed-Solomon code class.
/*!
    GF(256).
    D = 17.
    Generator polynomial: G=0,121,106,110,113,107,167,83,11,100,201,158,181,195,208,240,136.
 */
class RSEncode16
{
protected:
    static const uint8_t ginv[256];      ///< Inversion table.
    static const uint8_t galfa[255];     ///< Power table.
    static const uint8_t gmul[256][256]; ///< Multiplication table.

    /// Generator polynomial.
    static const uint8_t m_G[16];

    /// Remainder from division by the generator polynomial.
    /*!
        \param[in] data pointer to the polynomial.
        \param[out] data_mod pointer to the remainder (size 16).
        \param[in] size size of the polynomial.
    */
    void poly_remainder(uint8_t *data, uint8_t *data_mod, uint32_t size);
    /// Polynomial evaluation.
    /*!
        \param[in] alfa degree of alpha for the polynomial input value.
        \param[in] data_poly pointer to the polynomial.
        \param[in] size size of the polynomial.
        \return evaluation result.
    */
    uint8_t poly_eval(uint8_t alfa, uint8_t *data_poly, uint32_t size);
    /// Polynomial evaluation.
    /*!
        \param[in] data input value for the polynomial.
        \param[in] data_poly pointer to the polynomial.
        \param[in] size size of the polynomial.
        \return evaluation result.
    */
    uint8_t poly_eval2(uint8_t data, uint8_t *data_poly, uint32_t size);
    /// Polynomial multiplication.
    /*!
        \param[in] p1 pointer to the 1st polynomial.
        \param[in] p1_size size of the 1st polynomial.
        \param[in] p2 pointer to the 2nd polynomial.
        \param[in] p2_size size of the 2nd polynomial.
        \param[out] result pointer to the result.
        \param[in] result_size maximum result size.
    */
    void poly_mul(uint8_t *p1, uint32_t p1_size, uint8_t *p2, uint32_t p2_size, uint8_t *result, uint32_t result_size);

public:
    /// Encoding.
    /*!
        \param[in] data_in pointer to the input data array.
        \param[in] size size of the input data.
        \param[out] data_out pointer to the output data array (size of input + 16).
    */
    void encode(uint8_t *data_in, uint32_t size, uint8_t *data_out);
    /// Decoding.
    /*!
        \param[in] data_in pointer to the input data array (size of output + 16).
        \param[out] data_out pointer to the output data array.
        \param[in] size size of the output data.
    */
    void decode(uint8_t *data_in, uint8_t *data_out, uint32_t size);
};