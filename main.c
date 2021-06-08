// Convert binary file to Radio 86RK programm file.
// (c) 1-11-2017 Aleksey Morozov (Alemorf)
// AS IS

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
//#include <endian.h>
#include <errno.h>

#pragma pack(push, 1)

typedef struct
{
    uint16_t first_address_be;
    uint16_t last_address_be;
} header_t;

typedef struct
{
    uint32_t magic_E6000000;
    uint16_t check_sum_be;
} footer_t;

typedef struct
{
    header_t header;
    uint8_t  body[65536];
    footer_t reserved;
} programm_t;

uint16_t htobe16(uint16_t val)
{
	return (val >> 8) | ((val&0xFF)<<8);
}

#pragma pack(pop)

uint16_t calcSpecialistCheckSum(uint8_t* data, uint8_t* end)
{
    uint16_t s = 0;
    if(data == end) return s;
    end--;
    while(data != end)
        s += *data++ * 257;
    return (s & 0xFF00) + ((s + *data) & 0xFF);
}

int main(int argc, char** argv)
{
    unsigned start;
    ssize_t body_size;
    size_t write_size;
    int s, d;
    programm_t programm;

    if(argc != 4)
    {
        fprintf(stderr, "using: %s <load address in hex> <source file name> <dest file name>\n", argv[0]);
        return 1;
    }

    if(sscanf(argv[1], "%x%c", &start, argv[2]) != 1 || start > 0xFFFF)
    {
        fprintf(stderr, "incorrect address (%s)\n", argv[1]);
        return 1;
    }

    s = open(argv[2], O_RDONLY);
    if(s == -1) 
    {
        fprintf(stderr, "cannot open input file (%i)\n", errno);
        return 1;
    }

    d = creat(argv[3], 0666);
    if(d == -1)
    {
        fprintf(stderr, "cannot create output file (%i)\n", errno);
        close(s);
        return 1;
    }
    
    body_size = read(s, programm.body, sizeof(programm.body));
    if(body_size < 0) 
    {
        fprintf(stderr, "cannot read input file (%i)\n", errno);
        close(s);
        close(d);
        return 1;
    }

    if(read(s, programm.body, 1) != 0)
    {
        fprintf(stderr, "too big input file\n");
        close(s);
        close(d);
        return 1;
    }

    programm.header.first_address_be = htobe16(start);
    programm.header.last_address_be = htobe16(start + body_size - 1);

    footer_t* tail = (footer_t*)(programm.body + body_size);
    tail->magic_E6000000 = 0xE6000000;
    tail->check_sum_be = htobe16(calcSpecialistCheckSum(programm.body, programm.body + body_size));

    write_size = sizeof(header_t) + (size_t)body_size + sizeof(footer_t);
    if(write(d, &programm, write_size) != write_size)
    {
        fprintf(stderr, "cannot write output file (%i)\n", errno);
        close(s);
        close(d);
        return 1;
    }

    close(d);
    close(s);
    return 0;
}