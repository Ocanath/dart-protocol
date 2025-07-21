#ifndef SERIAL_COMMS_H  
#define SERIAL_COMMS_H
#include <stdint.h>
#include <stddef.h>

/*
	TODO: put the information in this comment into the README
	
	WE ARE ALWAYS LITTLE ENDIAN.
	This library is meant to be C-standard compliant, but to be used in a way which is technically NOT standard compliant (type punning).
	Specifically, if you are targeting little-endian 32bit architectures, such as an ARM STM32, you *can* set this up so that the generic memory
	regions you are reading and writing to are typedef structs (with the packed attribute if desired). This means you don't need any additional
	parsing layer - once you load the memory from an incoming write request frame, it is immediately accessible via the struct.

	The rationale is that:
		1. if you control the target architecture and it never changes, this optimization will always work
		2. DX improves drastically if you don't have to implement explicit parsing. Laying out your memory as a struct means you never have to write parsers for general settings messages - 
			only for very tight and specific message types!
*/

/*
	TODO:
		unit test set_rw and set_index. Make sure they are order independent as desired
		unit test the forward and reverse functions (msg to buf and buf to msg)
		refactor the parse function to use the above
		refactor the unit tests to use only the new functions; remove create_msic_write, create_misc_read, and parse_misc_command. Keep parse_general and index_of_field. Fix warnings in index_of_field - ensure casting is explicit
*/

#define NUM_BYTES_ADDRESS sizeof(unsigned char)
#define NUM_BYTES_INDEX sizeof(uint16_t)
#define NUM_BYTES_NUMWORDS_READREQUEST	sizeof(uint16_t)	//for a read struct request, we send a fixed 16bit integer argument in the payload section for the readsize request
#define NUM_BYTES_CHECKSUM sizeof(uint16_t)
#define MINIMUM_MESSAGE_LENGTH NUM_BYTES_NON_PAYLOAD

//This is a fixed address that always corresponds
#define MASTER_MOTOR_ADDRESS	0x7F
#define MASTER_MISC_ADDRESS		(0xFF - 0x7F)

#define READ_WRITE_BITMASK	0x8000	//msg is the read write bit. 1 for read, 0 for write.

enum {ERROR_MEMORY_OVERRUN = -5, ERROR_INVALID_ARGUMENT = -4, ERROR_CHECKSUM_MISMATCH = -3, ERROR_MALFORMED_MESSAGE = -2, ADDRESS_FILTERED = -1, SERIAL_PROTOCOL_SUCCESS = 0};

/*
 * Flags to capture byte field definitions for different physical and link layer protocols,
 	such as UART, CAN, FDCAN, UDP, SPI, and I2C.
 */
typedef enum 
{
	TYPE_SERIAL_MESSAGE = 0, 	//raw serial bytes. Must include our own address filtering and CRC appending. Examples: UART, RS485, RS232
	TYPE_ADDR_MESSAGE = 1,	//built-in addressing, but no build-in CRC. CRC must be added to the payload Examples: SPI, I2C
	TYPE_ADDR_CRC_MESSAGE = 2,	//built-in address filtering and CRC filtering-no address or crc fields required in payload. Examples: CAN, UDP
	
} serial_message_type_t;	

typedef enum {WRITE_MESSAGE, READ_MESSAGE} read_write_type_t;

typedef struct buffer_t
{
	unsigned char * buf;
	size_t size;	//size of the buffer
	size_t len;	//length of the current message in the buffer, referenced to the zero index
} buffer_t;

/*
Master write message/write request
 */
typedef struct misc_write_message_t
{
	unsigned char address;		//slave destination address
	uint16_t index;		//32bit-aligned index offset, where we want the payload to start writing to
	buffer_t payload;	//the content of the message, bytewise, which we will be writing
	//the checksum is computed in the message loader function, iff the hardware doesn't support it inherently. Therefore it is considered 'user alterable' data and not part of the message structure.
}misc_write_message_t;

/*
Master read message/read request
 */
typedef struct misc_read_message_t
{
	unsigned char address;		//slave destination address
	uint16_t index;		//32bit-aligned index offset, where we want the payload to start reading from
	uint16_t num_bytes;	//2^16 byte read requests at a time maximum. Not recommended to use buffers this large. 
}misc_read_message_t;

/*
Slave reply message
 */
typedef struct misc_reply_t
{
	unsigned char address;	
	buffer_t reply;
}misc_reply_t;

// // Consider: Context struct
// typedef struct 
// {
// 	unsigned char address;
// 	serial_message_type_t type;
// 	buffer_t * mem_base;
// } protocol_context_t;

int create_misc_write_message(unsigned char address, uint16_t index, buffer_t * payload, buffer_t * msg);
int create_misc_read_message(unsigned char address, uint16_t index, uint16_t num_words, buffer_t * msg);
int parse_general_message(unsigned char address, buffer_t * input, serial_message_type_t type, buffer_t * mem_base, buffer_t * reply);
int parse_misc_command(buffer_t * msg, serial_message_type_t type, buffer_t * reply, void * mem, size_t mem_size);
int index_of_field(void * p_field, void * mem, size_t mem_size);

#endif

