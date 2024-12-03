#include "automatic_fsm.h"

extern ADC_HandleTypeDef hadc1;

extern UART_HandleTypeDef huart2;

int status1 = INIT;
float ADC_value = 0;

uint8_t buffer_byte;
uint8_t buffer[MAX_BUFFER_SIZE];
uint8_t index_buffer = 0;
uint8_t buffer_flag = 0;

uint8_t cmd_flag = INIT;
uint8_t cmd_data[MAX_CMD_SIZE];
uint8_t cmd_data_index = 0;

int isCmdEqualToRST(uint8_t str[]) {
	if (str[0] == 'R' && str[1] == 'S' && str[2] == 'T')
		return 1;
	else
		return 	0;
}

int isCmdEqualToOK(uint8_t str[]) {
	if (str[0] == 'O' && str[1] == 'K')
		return 1;
	else
		return 0;
}

void cmd_parser_fsm() {
	switch (status1) {
	case INIT:
		if (buffer_byte == '!')
			status1 = READING;
		break;
	case READING:
		if (buffer_byte != '!' && buffer_byte != '#') {
			cmd_data[cmd_data_index] = buffer_byte;
			cmd_data_index++;
		}
		if (buffer_byte == '#') {
			status1 = STOP;
			cmd_data_index = 0;
		}
		break;
	case STOP:
		if (isCmdEqualToRST(cmd_data) == 1) {
			cmd_flag = RST;
			setTimer1(1);
		} else if (isCmdEqualToOK(cmd_data) == 1) {
			cmd_flag = OK;
		}
		status1 = INIT;
		break;
	default:
		break;
	}
}

void uart_comms_fsm() {
    static char last_packet[50];  // Lưu gói tin cuối cùng
    static float adc_stored_value = 0; // Lưu giá trị ADC trước đó
    static int adc_locked = 0; // Cờ khóa giá trị ADC
    char str[50];

    switch (cmd_flag) {
    case RST:
        if (!adc_locked) {
            // Đọc giá trị ADC khi nhận lệnh RST lần đầu
            adc_stored_value = HAL_ADC_GetValue(&hadc1) * 5.0 / 4096;
            adc_locked = 1;  // Khóa giá trị ADC
        }

        if (timer1_flag == 1) {
            int int_part = (int)adc_stored_value;
            int dec_part = (int)((adc_stored_value - int_part) * 100);
            sprintf(last_packet, "!ADC=%d.%02d#\r\n", int_part, dec_part);
            HAL_UART_Transmit(&huart2, (uint8_t*)last_packet, strlen(last_packet), 500);
            setTimer1(300);  // 3 giây
            timer1_flag = 0;
        }
        break;

    case OK:
        adc_locked = 0;  // Mở khóa cho phép cập nhật giá trị ADC mới
        HAL_UART_Transmit(&huart2, (uint8_t*)"STOP#\r\n", strlen("STOP#\r\n"), 500);
        cmd_flag = INIT;  // Reset trạng thái FSM
        break;

    default:
        break;
    }
}





