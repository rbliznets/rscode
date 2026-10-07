/*!
	\file
	\brief Модульные тесты.
   \authors Близнец Р.А. (r.bliznets@gmail.com)
	\version 0.0.0.1
	\date 05.05.2022
*/

#include <limits.h>
#include <cstring>
#include <cstdio>
#include "unity.h"
#include "RSEncode16.h"
#include "CTrace.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

#define countof(x) (sizeof(x)/sizeof(x[0]))

TEST_CASE("RSEncode16", "[encode][decode][fec]")
{
   uint32_t mem1=esp_get_free_heap_size();

   RSEncode16* enc=nullptr;
   enc = new RSEncode16();
   TEST_ASSERT_NOT_EQUAL(nullptr, enc);

   uint8_t dt1[120];
   for(uint8_t i = 0; i< countof(dt1);i++)
   {
	   dt1[i]=i+1;
   }
   uint8_t dt2[136];
   uint8_t dt3[countof(dt1)];

   STARTTIMESHOT();
   enc->encode(dt1,countof(dt1),dt2);
   STOPTIMESHOT("encode time");
   
   dt2[0]^=0x01;
   dt2[1]^=0x71;
   dt2[2]^=0x71;
   dt2[3]^=0x71;
   dt2[4]^=0x71;
   dt2[30]^=0x71;
   dt2[31]^=0x71;
   dt2[32]^=0x71;
   STARTTIMESHOT();
   enc->decode(dt2,dt3,countof(dt3));
   STOPTIMESHOT("decode time");
   TEST_ASSERT_EQUAL_UINT8_ARRAY(dt1, dt3, countof(dt1));

   delete enc;

   uint32_t mem2=esp_get_free_heap_size();
   if(mem1 != mem2)
   {
      TRACE("memory leak",mem1-mem2,false);
      TRACE("start",mem1,false);
      TRACE("stop",mem2,false);
      TEST_FAIL_MESSAGE("memory leak");
   }
}

namespace
{
   /// Воспроизводимый генератор (xorshift32): упавший тест повторяется точно.
   uint32_t rnd(uint32_t &state)
   {
      state ^= state << 13;
      state ^= state >> 17;
      state ^= state << 5;
      return state;
   }
}

/// Сверка с эталоном и случайные ошибки.
/*!
   Нужна прежде всего для CONFIG_RS_PIE: результат обязан побайтово совпадать
   со скалярной версией. Эталонные ECC посчитаны на хосте скалярным
   poly_remainder по таблицам table256.cpp.
*/
TEST_CASE("RSEncode16 vectors", "[encode][decode][fec]")
{
   static const uint8_t ecc24[16] = {0x01, 0x91, 0x96, 0xd8, 0xb2, 0x51, 0xf0, 0xb6, 0x5f, 0x49, 0xe0, 0xae, 0x48, 0x0f, 0xfa, 0x8f};
   static const uint8_t ecc38[16] = {0x76, 0xab, 0xe6, 0xd5, 0xbe, 0x5d, 0x49, 0xad, 0x63, 0x2a, 0x6b, 0x7a, 0xee, 0x7e, 0xd0, 0x6e};
   static const uint8_t ecc46[16] = {0xec, 0xb0, 0x0e, 0xec, 0x4d, 0xda, 0x68, 0xb7, 0xe0, 0xa6, 0xc4, 0xb7, 0x26, 0xb7, 0x3b, 0xee};
   static const uint8_t ecc120[16] = {0x3d, 0x23, 0xbc, 0x1e, 0xd4, 0x77, 0x66, 0x85, 0xd6, 0xf6, 0xd4, 0x5f, 0x52, 0x9b, 0xc7, 0x90};

   struct TVector
   {
      uint32_t size;       ///< Размер данных (размеры пакетов модема).
      const uint8_t *ecc;  ///< Эталонные 16 байт ECC для данных in[i] = i + 1.
   };
   static const TVector vectors[] = {{24, ecc24}, {38, ecc38}, {46, ecc46}, {120, ecc120}};

   RSEncode16 enc;
   uint8_t in[120];
   uint8_t out[136];
   uint8_t dec[120];

   for (auto &v : vectors)
   {
      for (uint32_t i = 0; i < v.size; i++)
         in[i] = i + 1;

      /// Кодирование: ECC обязаны совпасть с эталоном.
      enc.encode(in, v.size, out);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(v.ecc, &out[v.size], 16);

      /// Неискажённое кодовое слово декодируется без изменений.
      enc.decode(out, dec, v.size);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(in, dec, v.size);
   }

   /// Случайные данные и от 1 до 8 ошибок (предел кода) в случайных позициях.
   uint32_t seed = 0x12345678;
   for (uint32_t trial = 0; trial < 100; trial++)
   {
      for (auto &v : vectors)
      {
         for (uint32_t i = 0; i < v.size; i++)
            in[i] = (uint8_t)rnd(seed);
         enc.encode(in, v.size, out);

         uint8_t pos[8];
         uint32_t nerr = 1 + (rnd(seed) % 8);
         for (uint32_t e = 0; e < nerr; e++)
         {
            bool repeat;
            do
            {
               pos[e] = (uint8_t)(rnd(seed) % (v.size + 16));
               repeat = false;
               for (uint32_t k = 0; k < e; k++)
               {
                  if (pos[k] == pos[e])
                     repeat = true;
               }
            } while (repeat);

            uint8_t err = (uint8_t)rnd(seed);
            if (err == 0)
               err = 1;
            out[pos[e]] ^= err;
         }

         enc.decode(out, dec, v.size);
         TEST_ASSERT_EQUAL_UINT8_ARRAY(in, dec, v.size);
      }
   }

   /// Время, усреднённое по 100 проходам (одиночный замер шумит на таких размерах).
   for (uint32_t i = 0; i < 120; i++)
      in[i] = i + 1;
   STARTTIMESHOT();
   for (uint32_t n = 0; n < 100; n++)
      enc.encode(in, 120, out);
   STOPTIME("encode 120 time", 100);

   enc.encode(in, 120, out);
   out[5] ^= 0x71;
   out[40] ^= 0x71;
   out[99] ^= 0x71;
   STARTTIMESHOT();
   for (uint32_t n = 0; n < 100; n++)
      enc.decode(out, dec, 120);
   STOPTIME("decode 120 time", 100);
   TEST_ASSERT_EQUAL_UINT8_ARRAY(in, dec, 120);
}

#ifdef CONFIG_SPIRAM
/// Время decode() при прогретом и при вытесненном кэше данных.
/*!
   Нужно для выбора CONFIG_RS_GMUL_IN_RAM: без неё таблица gmul читается из флеша через кэш
   данных, и время декодирования зависит от того, что в кэше осталось. В прошивке между
   блоками кэш занят другими данными, поэтому в «холодном» замере перед каждым decode() кэш
   вытесняется чтением буфера в PSRAM вчетверо больше кэша. Истинное время лежит между
   двумя замерами. Данные случайные, в обоих замерах и в обеих сборках одни и те же.
*/
TEST_CASE("RSEncode16 decode cache", "[decode][fec]")
{
   static const uint32_t cErrors[] = {0, 1, 3, 8}; ///< Число искажённых байт в блоке.
   const uint32_t runs = 100;
   const uint32_t evictSize = 4 * CONFIG_ESP32S3_DATA_CACHE_SIZE;

   volatile uint8_t *evict = (volatile uint8_t *)heap_caps_malloc(evictSize, MALLOC_CAP_SPIRAM);
   TEST_ASSERT_NOT_NULL(evict);
   std::memset((void *)evict, 0x5a, evictSize);

   RSEncode16 enc;
   uint8_t in[120];
   uint8_t out[136];
   uint8_t dec[120];

   printf("RS gmul in %s\n",
#ifdef CONFIG_RS_GMUL_IN_RAM
          "RAM"
#else
          "flash"
#endif
   );
   for (uint32_t nerr : cErrors)
   {
      /// pass 0 - прогрев без замера, 1 - прогретый кэш, 2 - кэш вытеснен перед каждым блоком.
      for (uint32_t pass = 0; pass < 3; pass++)
      {
         uint32_t seed = 0x2401 + nerr;
         int64_t sum = 0, tmin = INT_MAX, tmax = 0;
         for (uint32_t n = 0; n < runs; n++)
         {
            for (uint32_t i = 0; i < 120; i++)
               in[i] = (uint8_t)rnd(seed);
            enc.encode(in, 120, out);

            uint8_t pos[8];
            for (uint32_t e = 0; e < nerr; e++)
            {
               bool repeat;
               do
               {
                  pos[e] = (uint8_t)(rnd(seed) % 136);
                  repeat = false;
                  for (uint32_t k = 0; k < e; k++)
                  {
                     if (pos[k] == pos[e])
                        repeat = true;
                  }
               } while (repeat);
               out[pos[e]] ^= (uint8_t)(rnd(seed) | 1);
            }

            if (pass == 2)
            {
               uint32_t x = 0;
               for (uint32_t i = 0; i < evictSize; i += CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE)
                  x += evict[i];
               TEST_ASSERT_EQUAL_UINT32((evictSize / CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE) * 0x5a, x);
            }

            int64_t t = esp_timer_get_time();
            enc.decode(out, dec, 120);
            t = esp_timer_get_time() - t;
            TEST_ASSERT_EQUAL_UINT8_ARRAY(in, dec, 120);

            sum += t;
            if (t < tmin)
               tmin = t;
            if (t > tmax)
               tmax = t;
         }
         if (pass != 0)
            printf("RS decode 120, errors %u, cache %s: min %d avg %d max %d usec\n", (unsigned)nerr, (pass == 1) ? "warm" : "cold",
                   (int)tmin, (int)(sum / runs), (int)tmax);
      }
   }

   heap_caps_free((void *)evict);
}
#endif // CONFIG_SPIRAM
