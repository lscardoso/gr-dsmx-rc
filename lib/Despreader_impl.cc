/* -*- c++ -*- */
/*
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "Despreader_impl.h"



namespace gr {
  namespace dsmx {

    Despreader::sptr
    Despreader::make()
    {
      return gnuradio::get_initial_sptr
        (new Despreader_impl());
    }

    void rotateLeft(unsigned char code[8]){

      uint8_t tmp = code[0];
      for(int i = 0; i< 7 ;i++){
        code[i] = (code[i]<<1 | code[i+1]>>(8-1));
      }
      code[7] = code[7]<<1 | tmp>>(8-1);
    }
    void rotateLeft(uint64_t* code){

      *code = (*code << 1) | (*code >> (64-1));
    }

    //Reverse bit order in one byte
    uint8_t Despreader_impl::reverse(uint8_t b) {
      b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
      b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
      b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
      return b;
    }

    uint16_t Despreader_impl::reverse_bytes(uint16_t d) {
      uint16_t b = d & 0xFF;
      b = b << 8;
      b = b | ((d>>8) & 0xFF);

      return b;
    }

    uint16_t Despreader_impl::reverse_bits_in_bytes(uint16_t d) {
      uint16_t b = Despreader_impl::reverse((uint8_t)( (d>>8) & 0xFF));
      b = b << 8;
      b = b | Despreader_impl::reverse((uint8_t)( d & 0xFF) );

      return b;
    }

    //Reverse  both byte order and bits in each byte
    uint16_t Despreader_impl::reverse_all(uint16_t d) {
      uint16_t b = Despreader_impl::reverse((uint8_t)(d & 0xFF));
      b = b << 8;
      b = b | Despreader_impl::reverse((uint8_t)( (d>>8) & 0xFF) );

      return b;
    }

    //Attempt in crc bit order switch to decode it unsuccessful so far
    uint16_t Despreader_impl::switch_to_crc_pn_scheme(uint16_t d) {
      printf("d: %04X\n", d);
      uint16_t b = ~Despreader_impl::reverse((uint8_t)( (d>>8) & 0xFF));
      //printf("reverse NOT:%02X\n", b);
      b = (b & 0x00FC) | ((b & 0x01)<<1) | ((b>>1) & 0x01);    // switch last two bits
      //printf("Switch: %02X\n", b);
      b = b << 8;
      //printf("Move: %04X\n", b);
      uint16_t c = Despreader_impl::reverse((uint8_t)( d & 0xFF) );
      //printf("reverse c:%02X from %02X \n", c, (uint8_t)( d & 0xFF));
      c = ~c;
      //printf("reverse NOT c:%02X\n", c);
      c = (c & 0xFFFC) | ((c & 0x01)<<1) | ((c>>1) & 0x01);    // switch last two bits
      //printf("Switch c: %02X\n", c);
      b = b | (c&0xFF);
      /*
      uint8_t a = (((d >>8) & 0x3F)<< 2);
      a = a | ((~(d >> 14) &0x03));
      uint8_t c = ((~(d>>6) &0x03)) | ((d & 0x3F) << 2);

      uint16_t b = a;
      b = b << 8;
      b = b | c;*/
      return b;
    }



    //Find if data matches a given code. Outputs the number of bits the code is rotated
    int correlate(uint64_t code, uint64_t data, unsigned char threshold){
      threshold = 64 - threshold;
      int count=0;

      for(int index=0; index<64;index++){
        count = 0;

        for(int i = 0; i<64; i++){
          if( ((code >> i)& 1) == ((data >> i)& 1) ){
            count++;
          }
        }
        if( count >= threshold){
          return index;
        }
        rotateLeft(&code);
      }
      return -1;
    }

    //Despreads data
    int decodeByte(uint64_t data, uint64_t pn_data0, uint64_t pn_data1){
      int offset = 0;
      int shift;
      unsigned char threshold = 10;
      shift = correlate(~pn_data0, data, threshold);
      if(shift < 0){
        offset = 64;
        shift = correlate(pn_data0, data, threshold);
        if (shift < 0) {
          offset = 128;
          shift = correlate(~pn_data1, data, threshold);
          if(shift < 0){
            offset = 192;
            shift = correlate(pn_data1, data, threshold);
          }
        }
      }
      if(shift >= 0){
        return shift + offset;
      }
      return -1;

    }


    uint64_t Despreader_impl::cast864(uint8_t data[8]){
      uint64_t casted=0;
      for(int i = 0; i<8;i++){
        casted = (casted<<8) | data[i];
      }
      return casted;
    }
    uint64_t Despreader_impl::cast864reverse(uint8_t data[8]){
      uint64_t casted=0;
      for(int i = 0; i<8;i++){
        casted = (casted<<8) | Despreader_impl::reverse(data[i]);
      }
      return casted;
    }


    void Despreader_impl::callback(pmt::pmt_t msg){
      pmt::pmt_t meta = pmt::car(msg);
      pmt::pmt_t vector = pmt::cdr(msg);

      std::cout << "* MESSAGE DEBUG PRINT PDU VERBOSE *\n";

      pmt::print(meta);

      d_row = pmt::to_long(pmt::dict_ref(meta, pmt::intern("Row"), pmt::PMT_NIL));
      d_column = pmt::to_long(pmt::dict_ref(meta, pmt::intern("Column"), pmt::PMT_NIL));
      d_channel = pmt::to_long(pmt::dict_ref(meta, pmt::intern("Channel"), pmt::PMT_NIL));
      int data_col0 = 7 - d_column;
      int data_col1 = data_col0 + 1;
      printf("SOP column: %d, data_col0: %d, data_col1: %d\n", d_column, data_col0, data_col1);

      size_t len = pmt::blob_length(vector);
      std::cout << "pdu_length = " << len << std::endl;
      std::cout << "contents = " << std::endl;
      size_t offset(0);     //Data recovery
      uint8_t* data = (uint8_t*) pmt::uniform_vector_elements(vector, offset);
      for(size_t i=0; i<len; i+=16){      //Print raw chips
        printf("%04x: ", ((unsigned int)i));
        for(size_t j=i; j<std::min(i+16,len); j++){
          printf("%02x ",data[j] );
        }

        std::cout << std::endl;
      }
      std::cout << "\n" << std::endl;

                //--------- Decoding
      uint8_t length = 0;
      for(size_t i=0; i<len; i+=8){
        int decoded = decodeByte(cast864(&data[i]), cast864reverse(pncodes[d_row][data_col0]), cast864reverse(pncodes[d_row][data_col1]) );
        if (decoded == -1) {
          for (size_t row = 0; row < 5; row++) {
            decoded = decodeByte(cast864(&data[i]), cast864reverse(pncodes[row][data_col0]), cast864reverse(pncodes[row][data_col1]) );
            if (decoded != -1){
              printf("Alternate decoding column found: %d should be different from %d\n", row, d_row);
              break;
            }
          }
          if (decoded == -1) {
            printf("%s\n", "Decoding failure");
            return;
          }
        }

        printf("%02x",decoded);         //Print des données décodées 2 octets par ligne
        if(i % (8*2) == 0){
        }         //--------Dispatch to various buffers
        if(i==0){                       //First Byte is length
          length = decoded;
        }
        else{
          if(i < (8* (16+1))){             //16 bytes of data, each made from 8 bytes of chips and offset of one byte for the length byte
            if (i % (8*2) != 0)       //We want to regroup data 2 bytes by 2 bytes, stating after the first byte of length
              d_data_chunks[i / (8*2)] = decoded;
            else
              d_data_chunks[i / (8*2) - 1] = (d_data_chunks[i / (8*2) - 1] << 8) | decoded;
          }
          else                        //2 bytes of CRC
            d_crc_recieved = (d_crc_recieved << 8) | decoded;
        }
      }
      //-----------Print tentative experimental pn scheme
      std::cout << std::endl;
      /*for (size_t i = 0; i < 8; i++) {
        printf("%04X\n", Despreader_impl::switch_to_crc_pn_scheme(d_data_chunks[i]));
      }*/

      //-----------Print channel data
      for(int i=1;i<8;i++){
        printf("Packet: %u, Channel: %u, Value: %f\n", d_data_chunks[i]>>15, (d_data_chunks[i]>>11) & 0xF, ((d_data_chunks[i] & 0x7FF)/1024.0) - 1);
      }
      //uint16_t test[8] = {0x1234,0x5678,0x9101,0x1121,0x3141,0x5161,0x7181,0x9202};
      //printf("Test CRC: %04X\n", Despreader_impl::crc_calc(test,0x10,0x0000,false));

      //----------CRC to CRC experimental pn scheme
      /*printf(" Before %04X\n", d_crc_recieved);
      d_crc_recieved = Despreader_impl::switch_to_crc_pn_scheme(d_crc_recieved);
      printf(" After %04X\n", d_crc_recieved );*/

      uint16_t id_bytes01 = Despreader_impl::crc_seed_find(d_data_chunks, length, d_crc_recieved);


      //----------Channel sequence
      if(id_bytes01 == 0){//Should be if id != 0

        Despreader_impl::calc_dsmx_channel(/*id_bytes01*/0x49B6, d_data_chunks[0]);
        int next_channel = Despreader_impl::get_next_channel(d_channel);              //Send all info in message: next channel, radio id and decoded channel values + channel sequence in data vector
        d_pdu_meta = pmt::make_dict();
        d_pdu_meta = dict_add(d_pdu_meta, pmt::intern("Next channel"), pmt::mp(next_channel));
        d_pdu_meta = dict_add(d_pdu_meta, pmt::intern("Radio id1"), pmt::mp(/*id_bytes01*/0x49B6));
        d_pdu_meta = dict_add(d_pdu_meta, pmt::intern("Radio id2"), pmt::mp(d_data_chunks[0]));
        for (size_t i = 1; i < 8; i++) {
          char string[5];
          sprintf(string, "%d %d", (d_data_chunks[i]>>15), ((d_data_chunks[i]>>11) & 0xF));
          d_pdu_meta = dict_add(d_pdu_meta, pmt::intern(string), pmt::mp(d_data_chunks[i] & 0x7FF));
        }
        d_pdu_vector = gr::pdu::make_pdu_vector(d_type, d_channels, 23);
        pmt::pmt_t msg = pmt::cons(d_pdu_meta, d_pdu_vector);
        message_port_pub(pmt::mp("pdus"), msg);
        std::cout << std::endl;
      }

      std::cout << "***********************************\n";
    }

    uint16_t Despreader_impl::crc_seed_find(uint16_t data[8], uint8_t length, uint16_t transmitted){
      uint16_t seed = 0;
      uint16_t id_byte2 = ((data[0]>>8) & 0x00FF);
      uint16_t crc = 0;

      printf("Transmitted:                                                                        %04X\n", transmitted);
      for (uint16_t id_byte0 = 0; id_byte0 <= 0xFF; id_byte0++) {
        for (uint16_t id_byte1 = 0; id_byte1 <= 0xFF; id_byte1++) {
          if ( ((id_byte0 + id_byte1 + id_byte2 + 2) & 0x07) == (d_column & 0x07) ) { //We don't need to try every combination
            if (id_byte0 == 0x49 && id_byte1 == 0xB6) {         //Debugging test
              printf("%s\n", "On teste bien cette possibilité");
            }
            else
              continue;
            seed = ~((id_byte0<<8) | id_byte1);
            printf("seed: %04X\n", seed);
            crc = Despreader_impl::crc_calc(data,length, seed, false);
            printf("crc1 False: %04X\n", crc);
            //printf("Calculated: %04X \n", crc);
            if(transmitted == crc ){
              printf("Found01! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_bytes(crc) ){
              printf("Found02! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_bits_in_bytes(crc) ){
              printf("Found03! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_all(crc) ){
              printf("Found04! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }

            crc = ~Despreader_impl::crc_calc(data,length, seed, true);
            printf("Calculated: %04X \n", crc);
            if(transmitted == crc ){
              printf("Found11! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_bytes(crc) ){
              printf("Found12! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_bits_in_bytes(crc) ){
              printf("Found13! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_all(crc) ){
              printf("Found14! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }

            seed = ((id_byte0<<8) | id_byte1);
            printf("seed: %04X\n", seed);
            crc = Despreader_impl::crc_calc(data,length, seed, false);
            printf("Calculated False: %04X \n", crc);
            if(transmitted == crc ){
              printf("Found05! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_bytes(crc) ){
              printf("Found06! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_bits_in_bytes(crc) ){
              printf("Found07! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_all(crc) ){
              printf("Found08! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }

            crc = ~Despreader_impl::crc_calc(data,length, seed, true);
            printf("Calculated: %04X \n", crc);
            if(transmitted == crc ){
              printf("Found15! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_bytes(crc) ){
              printf("Found16! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_bits_in_bytes(crc) ){
              printf("Found17! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
            if(transmitted == Despreader_impl::reverse_all(crc) ){
              printf("Found18! : %02X%02X\n", id_byte1,id_byte0);
              //return ((id_byte0<<8) | id_byte1);
            }
          }
        }
      }
      return 0;
    }

    uint16_t Despreader_impl::crc_calc(uint16_t data[8], uint8_t length, uint16_t seed, bool reverse){

      Despreader_impl::crc_seed_set(seed);
      //Despreader_impl::crc_update(seed, reverse);
      Despreader_impl::crc_update(length, reverse);
      for (size_t i = 0; i < 8; i++) {
        Despreader_impl::crc_update(data[i], reverse);
      }
      //return ((d_reminder & 0xFF) << 8) | ((d_reminder >> 8) & 0xFF);
      return d_reminder;
      //seed = Despreader_impl::reverse_bits_in_bytes(seed);
      /*uint16_t crc = Despreader_impl::update_crc_16(seed, length, reverse);
      for (size_t i = 0; i < 8; i++) {
        crc = Despreader_impl::update_crc_16(crc, (data[i]>>8)&0xFF, reverse);
        crc = Despreader_impl::update_crc_16(crc, (data[i])&0xFF, reverse);
      }
      return crc;*/
    }

    void Despreader_impl::crc_seed_set(uint16_t seed){
      d_reminder = seed;
    }
    void Despreader_impl::crc_seed_set(uint8_t seed_MSB, uint8_t seed_LSB){
      d_reminder = (seed_MSB << 8) | seed_LSB;
    }

    void Despreader_impl::crc_update(uint8_t data, bool reverse){
      uint8_t data_bit=0;
      uint8_t xored=0;
      if(reverse) data=Despreader_impl::reverse(data);

      for(int i =0; i<8;i++){
        if ( (data >> i) & 0x01) {
          data_bit = 1;
        } else {
          data_bit = 0;
        }
        d_high_reminder = (d_reminder & 0x8000) >> 15;
        d_reminder = (d_reminder << 1) & 0xFFFE;


        xored = data_bit ^ d_high_reminder;
        d_reminder = d_reminder | xored;
        if( xored ){  //Check if current bit XOR high reminder is 1
          d_reminder = d_reminder ^ 0x8005;             //XOR with generator polynomial
        }
      }
    }
    void Despreader_impl::crc_update(uint16_t data, bool reverse){
      Despreader_impl::crc_update((uint8_t) ((data >> 8) & 0xFF), reverse);
      Despreader_impl::crc_update((uint8_t) (data & 0xFF), reverse);

    }

    /*
     * uint16_t update_crc_16( uint16_t crc, unsigned char c );
     *
     * The function update_crc_16() calculates a new CRC-16 value based on the
     * previous value of the CRC and the next byte of data to be checked.
     */

    uint16_t Despreader_impl::update_crc_16( uint16_t crc, unsigned char c , bool reverse) {

    	if ( ! crc_tab16_init ) Despreader_impl::init_crc16_tab();
      if (reverse) c = Despreader_impl::reverse(c);

    	return (crc >> 8) ^ crc_tab16[ (crc ^ (uint16_t) c) & 0x00FF ];

    }  /* update_crc_16 */

    /*
     * static void init_crc16_tab( void );
     *
     * For optimal performance uses the CRC16 routine a lookup table with values
     * that can be used directly in the XOR arithmetic in the algorithm. This
     * lookup table is calculated by the init_crc16_tab() routine, the first time
     * the CRC function is called.
     */

    void Despreader_impl::init_crc16_tab( void ) {

    	uint16_t i;
    	uint16_t j;
    	uint16_t crc;
    	uint16_t c;

    	for (i=0; i<256; i++) {

    		crc = 0;
    		c   = i;

    		for (j=0; j<8; j++) {

    			if ( (crc ^ c) & 0x0001 ) crc = ( crc >> 1 ) ^ 0x8005/*0xA001*/; // Ou 0xA001
    			else                      crc =   crc >> 1;

    			c = c >> 1;
    		}

    		crc_tab16[i] = crc;
    	}

    	crc_tab16_init = true;

    } /* init_crc16_tab */


    void Despreader_impl::calc_dsmx_channel(uint16_t byte01, uint16_t byte23){
      uint32_t idx = 0;
      uint32_t id = ~((byte01 << 16) | (byte23 << 0));
      uint32_t id_tmp = id;
      uint32_t count_3_27 = 0, count_28_51 = 0, count_52_76 = 0;
      while(idx < 23) {
        uint32_t i;
        id_tmp = id_tmp * 0x0019660D + 0x3C6EF35F; // Randomization
        uint32_t next_ch = ((id_tmp >> 8) % 0x49) + 3;  // Use least-significant byte and must be larger than 3
        if (((next_ch ^ id) & 0x01) == 0)
            continue;
        for (i = 0; i < idx; i++) {
            if(d_channels[i] == next_ch)
                break;
        }
        if (i != idx)
            continue;
        if ((next_ch < 28 && count_3_27 < 8)
          ||(next_ch >= 28 && next_ch < 52 && count_28_51 < 7)
          ||(next_ch >= 52 && count_52_76 < 8))
        {
          if(next_ch <= 27)
              count_3_27++;
          else if (next_ch <= 51)
              count_28_51++;
          else
              count_52_76++;
          d_channels[idx++] = next_ch;
        }
      }
    }

    int Despreader_impl::get_next_channel(int current){
      for(int i = 0; i< 23; i++){
        if(d_channels[i] == current){
          return d_channels[ (i+1) % 23];
        }
      }
      return d_channels[0];
    }

    /*
     * The private constructor
     */
    Despreader_impl::Despreader_impl()
      : gr::sync_block("Despreader",
                 io_signature::make(0, 0, 0),
                 io_signature::make(0, 0, 0))
    {
      message_port_register_in(pmt::mp("Msg"));
      set_msg_handler(pmt::mp("Msg"), [this](pmt::pmt_t msg) { this->callback(msg); });
      crc_tab16_init = false;
      message_port_register_out(pmt::mp("pdus"));
      d_type = gr::types::byte_t;
      d_pdu_meta = pmt::PMT_NIL;
      d_pdu_vector = pmt::PMT_NIL;
    }

    /*
     * Our virtual destructor.
     */
    Despreader_impl::~Despreader_impl()
    {
    }


    int
    Despreader_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      /*const int *in = (const <+ITYPE+> *) input_items[0];
      <+OTYPE+> *out = (<+OTYPE+> *) output_items[0];*/

      // Do <+signal processing+>

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }





  } /* namespace dsmx */
} /* namespace gr */
