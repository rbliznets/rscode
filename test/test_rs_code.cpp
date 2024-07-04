/*!
	\file
	\brief Модульные тесты.
   \authors Близнец Р.А. (r.bliznets@gmail.com)
	\version 0.0.0.1
	\date 05.05.2022
	\copyright (c) Copyright 2022, ООО "Глобал Ориент", Москва, Россия, http://www.glorient.ru/
*/

#include <limits.h>
#include <cstring>
#include "unity.h"
#include "RSEncode16.h"
#include "CTrace.h"

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

