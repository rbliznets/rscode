/*!
    \file
    \brief Класс для реализации кодирования Рида-Соломона.
    \authors Близнец Р.А. (r.bliznets@gmail.com)
    \version 1.0.1.0
    \date 21.04.2021
*/

#include "RSEncode16.h"
#include <cstring>
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_log.h"

#ifdef CONFIG_RS_IN_RAM
IRAM_ATTR
#endif // CONFIG_RS_IN_RAM
void RSEncode16::poly_remainder(uint8_t *data, uint8_t *data_mod, uint32_t size)
{
    assert(data != nullptr);
    assert(data_mod != nullptr);
    assert(size > 16);

    uint8_t x = data[0];
    for (uint32_t j = 0; j < 16; j++)
    {
        data_mod[j] = data[j + 1] ^ gmul[x][m_G[j]];
    }

    for (uint32_t i = 1; i < size; i++)
    {
        x = data_mod[0];
        for (uint32_t j = 0; j < 15; j++)
        {
            data_mod[j] = data_mod[j + 1] ^ gmul[x][m_G[j]];
        }
        data_mod[15] = data[i + 16] ^ gmul[x][m_G[15]];
    }
}

#ifdef CONFIG_RS_IN_RAM
IRAM_ATTR
#endif // CONFIG_RS_IN_RAM
void RSEncode16::encode(uint8_t *data_in, uint32_t size, uint8_t *data_out)
{
    assert(data_in != nullptr);
    assert(data_out != nullptr);
    assert(size > 0);

    if (data_in != data_out)
        std::memcpy(data_out, data_in, size);
    std::memset(&data_out[size], 0, 16);

    uint8_t tmp[16];
    poly_remainder(data_out, tmp, size);
    std::memcpy(&data_out[size], tmp, 16);
}

#ifdef CONFIG_RS_IN_RAM
IRAM_ATTR
#endif // CONFIG_RS_IN_RAM
uint8_t RSEncode16::poly_eval(uint8_t alfa, uint8_t *data_poly, uint32_t size)
{
    assert(data_poly != nullptr);
    assert(size > 0);

    uint8_t x = galfa[alfa];
    uint8_t x1 = x;
    uint8_t s = data_poly[size - 1];
    for (uint32_t i = 0; i < (size - 1); i++)
    {
        s ^= gmul[data_poly[size - 2 - i]][x1];
        x1 = gmul[x1][x];
    }
    return s;
}

#ifdef CONFIG_RS_IN_RAM
IRAM_ATTR
#endif // CONFIG_RS_IN_RAM
uint8_t RSEncode16::poly_eval2(uint8_t data, uint8_t *data_poly, uint32_t size)
{
    assert(data_poly != nullptr);
    assert(size > 0);

    uint8_t s = data_poly[size - 1];
    for (uint32_t i = 0; i < (size - 1); i++)
    {
        s = gmul[s][data] ^ data_poly[size - 2 - i];
    }
    return gmul[s][data];
}

#ifdef CONFIG_RS_IN_RAM
IRAM_ATTR
#endif // CONFIG_RS_IN_RAM
void RSEncode16::decode(uint8_t *data_in, uint8_t *data_out, uint32_t size)
{
    assert(data_in != nullptr);
    assert(data_out != nullptr);
    assert(size > 0);

    if (data_in != data_out)
        std::memcpy(data_out, data_in, size);

    uint8_t tmp[16];
    poly_remainder(data_in, tmp, size);

    uint8_t s[16];
    uint8_t flag = 0;
    for (uint8_t i = 0; i < 16; i++)
    {
        s[i] = poly_eval(i + 1, tmp, 16);
        if (s[i] != 0)
            flag = 1;
    }
    if (flag == 0)
        return;

    uint8_t err_loc[9] = {1, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t sz_err = 1;
    uint8_t old_loc[9] = {1, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t sz_old = 1;
    uint8_t new_loc[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t sz_new;
    for (uint8_t i = 0; i < 16; i++)
    {
        uint8_t delta = s[i];
        for (uint8_t j = 1; j < sz_err; j++)
        {
            delta ^= gmul[err_loc[sz_err - j - 1]][s[i - j]];
        }
        sz_old++;

        if (delta != 0)
        {
            if (sz_old > sz_err)
            {
                for (uint8_t j = 0; j < sz_old; j++)
                {
                    new_loc[j] = gmul[delta][old_loc[j]];
                }
                sz_new = sz_old;

                for (uint8_t j = 0; j < sz_err; j++)
                {
                    old_loc[j] = gmul[ginv[delta]][err_loc[j]];
                }
                sz_old = sz_err;

                for (uint8_t j = 0; j < sz_new; j++)
                {
                    err_loc[j] = new_loc[j];
                }
                sz_err = sz_new;
            }

            uint8_t k = sz_err - sz_old;
            for (uint8_t j = k; j < sz_err; j++)
            {
                err_loc[j] ^= gmul[delta][old_loc[j - k]];
            }
        }
    }

    for (uint8_t j = 0; j < sz_err; j++)
    {
        new_loc[j] = err_loc[sz_err - 1 - j];
    }

    sz_old = 0;
    for (uint8_t j = 0; j < (size + 16); j++)
    {
        if (poly_eval(j, new_loc, sz_err) == 0)
        {
            old_loc[sz_old] = j;
            err_loc[sz_old] = size + 15 - j;
            sz_old++;
            if (sz_old == (sz_err - 1))
                break;
        }
    }

    // Omega(x) = [ Synd(x) * Error_loc(x) ] mod x^(n-k+1)
    uint8_t omega[8];
    std::memset(omega, 0, 8);
    poly_mul(s, 16, new_loc, sz_err, omega, sz_err - 1);

    for (uint8_t j = 0; j < sz_err - 1; j++)
    {
        old_loc[j] = galfa[old_loc[j]];
    }

    for (uint8_t i = 0; i < sz_err - 1; i++)
    {
        if (err_loc[i] < size)
        {
            uint8_t x_inv = ginv[old_loc[i]];
            uint8_t err_loc_prime = 1;
            for (uint8_t j = 0; j < sz_err - 1; j++)
            {
                if (i != j)
                {
                    uint8_t x = 1 ^ gmul[x_inv][old_loc[j]];
                    err_loc_prime = gmul[err_loc_prime][x];
                }
            }
            //    if(err_loc_prime == 0)
            //    {
            //         ESP_LOGW("RSEncode16","err_loc_prime == 0");
            //    }
            uint8_t y = poly_eval2(x_inv, omega, sz_err - 1);
            err_loc_prime = ginv[err_loc_prime];
            data_out[err_loc[i]] ^= gmul[y][err_loc_prime];
        }
    }
}

#ifdef CONFIG_RS_IN_RAM
IRAM_ATTR
#endif // CONFIG_RS_IN_RAM
void RSEncode16::poly_mul(uint8_t *p1, uint32_t p1_size, uint8_t *p2, uint32_t p2_size, uint8_t *result, uint32_t result_size)
{
    assert(p1 != nullptr);
    assert(p1_size > 0);
    assert(p2 != nullptr);
    assert(p2_size > 0);
    assert(result != nullptr);
    assert(result_size > 0);

    std::memset(result, 0, result_size);
    for (uint32_t n1 = 0; (n1 < result_size) && (n1 < p1_size); n1++)
    {
        for (uint32_t n2 = 0; n2 < p2_size; n2++)
        {
            if ((n1 + n2) >= result_size)
                break;
            result[n1 + n2] ^= gmul[p1[n1]][p2[n2]];
        }
    }
}
