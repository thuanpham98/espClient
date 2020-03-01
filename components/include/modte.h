#ifndef _MODTE_
#define _MODTE_


typedef struct Frame 
{
    uint16_t ID ;
    uint8_t opcode;
    uint8_t function;
    uint16_t data;
}frame_t;



esp_err_t analyst(char *pack);
esp_err_t packed (char *pack);
esp_err_t hello_te();

#endif