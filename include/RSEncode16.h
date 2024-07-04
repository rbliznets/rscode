/*!
	\file
    \brief Класс для реализации кодирования Рида-Соломона.
    \authors Близнец Р.А. (r.bliznets@gmail.com)
    \version 1.0.1.0
    \date 21.04.2021
*/

#pragma once

#include "sdkconfig.h"
#include <stdint.h>

/// Класс кода Рида-Соломона.
/*!
    GF(256).
    D = 17.
    Генерирующий полином: G=0,121,106,110,113,107,167,83,11,100,201,158,181,195,208,240,136.
 */
class RSEncode16
{
protected:
    static const uint8_t ginv[256];         ///< Таблица инверсии.
    static const uint8_t galfa[255];        ///< Таблица степеней.
    static const uint8_t gmul[256][256];    ///< Таблица умножения.

    /// Генерирующий полином.
    static const uint8_t m_G[16];

    /// Остаток от деления на генерирующий полином.
    /*!
        \param[in] data указатель на полином.
        \param[out] data_mod указатель на остаток (размер 16).
        \param[in] size размер полинома.
    */
    void poly_remainder(uint8_t* data, uint8_t* data_mod, uint32_t size);
    /// Вычисление полинома.
    /*!
        \param[in] alfa степень альфа входного значения полинома.
        \param[in] data_poly указатель на полином.
        \param[in] size размер полинома.
        \return результат вычисления.
    */
    uint8_t poly_eval(uint8_t alfa, uint8_t* data_poly, uint32_t size);
    /// Вычисление полинома.
    /*!
        \param[in] data входное значение полинома.
        \param[in] data_poly указатель на полином.
        \param[in] size размер полинома.
        \return результат вычисления.
    */
    uint8_t poly_eval2(uint8_t data, uint8_t* data_poly, uint32_t size);
    /// Умножение полиномов.
    /*!
        \param[in] p1 указатель на 1-й полином.
        \param[in] p1_size размер 1-го полинома.
        \param[in] p2 указатель на 2-ой полином.
        \param[in] p2_size размер 2-го полинома.
        \param[out] result указатель на результат.
        \param[in] result_size максимальный размер результата.
    */
    void poly_mul(uint8_t* p1, uint32_t p1_size, uint8_t* p2, uint32_t p2_size, uint8_t* result, uint32_t result_size);

public:
    /// Кодирование.
    /*!
        \param[in] data_in указатель на массив входных данных.
        \param[in] size размер входных данных.
        \param[out] data_out указатель на массив выходных данных (размер входных + 16).
    */
    void encode(uint8_t* data_in, uint32_t size, uint8_t* data_out);
    /// Декодирование.
    /*!
        \param[in] data_in указатель на массив входных данных (размер выходных + 16).
        \param[out] data_out указатель на массив выходных данных.
        \param[in] size размер выходных данных.
    */
    void decode(uint8_t* data_in, uint8_t* data_out, uint32_t size);

};

