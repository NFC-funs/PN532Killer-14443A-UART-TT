#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "Ntag.h"
#include "Ntag215.h"

extern uint8_t recv_data[];
uint8_t trans_data[1024];

uint8_t trans_header[3];

uint16_t __not_in_flash_func(crc16_A)(uint8_t *data, uint16_t len)
{
	uint32_t crc = 0x6363;
    uint8_t i;
    uint8_t bt;

	for(i=0; i<len; i++) {
		bt = data[i];

		bt = bt ^ ((uint16_t)crc);
		bt = bt ^ (bt << 4);

		crc = (crc >> 8) ^ ((((uint32_t)bt) << 8) ^ (((uint32_t)bt) << 3) ^ (((uint32_t)bt) >> 4));
	}

	return (((uint16_t)((crc & 0xFF)) << 8) | ((uint16_t)((crc >> 8) & 0xFF)));
}
void __not_in_flash_func(AppendCrc14443a)(uint8_t *data, uint16_t len)
{
	uint16_t crc;
	crc = crc16_A(data, len);
    data[len] = (crc >> 8) & 0xff;
    data[len+1] = crc & 0xff;
}
void __not_in_flash_func(X_TransHandle)(uint8_t type, uint16_t len)
{
	uint8_t sum;
	uint16_t i;

	sum = 0;
	//first transmit 3bytes
	//len
	trans_header[0] = (len>>8)&0xFF;
	sum += trans_header[0];
	trans_header[1] = len&0xFF;
	sum += trans_header[1];
	//type
	trans_header[2] = type;
	sum += trans_header[2];

	//data + sum
	for(i=0;i<len;i++) sum += trans_data[i];
	trans_data[len] = sum;

	uart_write_blocking(uart1, trans_header, 3);
	uart_write_blocking(uart1, trans_data, len+1);
	//printf("Rsp len %d\n", len);
} 

void __not_in_flash_func(X_Ntag_Emul)()
{
	uint16_t receivedCmd_len, i;

	receivedCmd_len = (recv_data[0]*256 + recv_data[1]);
	//X_TransHandle(0, 2);
	//return;

	// WUPA in HALTED state or REQA or WUPA in any other state
	if (receivedCmd_len == 1 && ((recv_data[3] == ISO14443A_CMD_REQA) || recv_data[3] == ISO14443A_CMD_WUPA))
	{
		//data
		trans_data[0] = Ntag215_Atqa[0];
		trans_data[1] = Ntag215_Atqa[1];

		X_TransHandle(0, 2);
		//return;
	}
	// select all - 0x93 0x20
	else if (receivedCmd_len == 2 && (recv_data[3] == ISO14443A_CMD_ANTICOLL_OR_SELECT && recv_data[4] == 0x20))
	{

		trans_data[0] = 0x88;
		trans_data[1] = Ntag215_Data[0];
		trans_data[2] = Ntag215_Data[1];
		trans_data[3] = Ntag215_Data[2];
		trans_data[4] = Ntag215_Data[3];

		X_TransHandle(0, 5);
		//return;
	}
	// select all - 0x95 0x20, PS-20250131
	else if (receivedCmd_len == 2 && (recv_data[3] == ISO14443A_CMD_ANTICOLL_OR_SELECT_2 && recv_data[4] == 0x20))
	{

		trans_data[0] = Ntag215_Data[4];
		trans_data[1] = Ntag215_Data[5];
		trans_data[2] = Ntag215_Data[6];
		trans_data[3] = Ntag215_Data[7];
		trans_data[4] = Ntag215_Data[8];

		X_TransHandle(0, 5);
		//return;
	}

	// select card - 0x93 0x70 ...
	else if (receivedCmd_len == 9 && (recv_data[3] == ISO14443A_CMD_ANTICOLL_OR_SELECT && recv_data[4] == 0x70 ))
	{
		trans_data[0] = Ntag215_Sak[0];
		AppendCrc14443a(trans_data, 1);
		X_TransHandle(0, 3);
		
		//return;
	}
	// select card - 0x95 0x70 ... PS-20250131
	else if (receivedCmd_len == 9 && (recv_data[3] == ISO14443A_CMD_ANTICOLL_OR_SELECT_2 && recv_data[4] == 0x70 ))
	{
		if (memcmp(&recv_data[5], &Ntag215_Data[4], 4) == 0)
		{
			trans_data[0] = Ntag215_Sak[1];
			AppendCrc14443a(trans_data, 1);
			X_TransHandle(0, 3);
		}
		//return;
	}
	// read block - 0x30 xx
	else if (receivedCmd_len == 4 && (recv_data[3] == NTAG_CMD_READ))
	{
		for (i=0;i<16;i++) trans_data[i] = Ntag215_Data[recv_data[4]*4 + i];
		AppendCrc14443a(trans_data, 16);
		X_TransHandle(0, 18);
		//return;
	}
	// get version - 0x60
	else if (receivedCmd_len == 3 && (trans_data[3] == NTAG_CMD_GET_VERSION))
	{
		trans_data[0] = Ntag215_Version[0];
		trans_data[1] = Ntag215_Version[1];
		trans_data[2] = Ntag215_Version[2];
		trans_data[3] = Ntag215_Version[3];
		trans_data[4] = Ntag215_Version[4];
		trans_data[5] = Ntag215_Version[5];
		trans_data[6] = Ntag215_Version[6];
		trans_data[7] = Ntag215_Version[7];
		AppendCrc14443a(trans_data, 8);
		X_TransHandle(0, 10);
		//return;
	}
}