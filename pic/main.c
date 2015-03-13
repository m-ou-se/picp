#include <USB/usb.h>
#include <USB/usb_function_cdc.h>

#pragma config FOSC = INTOSC
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = OFF
#pragma config CP = OFF
#pragma config BOREN = ON
#pragma config CLKOUTEN = OFF
#pragma config IESO = OFF
#pragma config FCMEN = OFF
#pragma config WRT = OFF
#pragma config CPUDIV = NOCLKDIV
#pragma config USBLSCLK = 48MHz
#pragma config PLLMULT = 3x
#pragma config PLLEN = ENABLED
#pragma config STVREN = ON
#pragma config BORV = LO
#pragma config LPBOR = OFF
#pragma config LVP = ON

enum {
	icsp_cmd_load_configuration                 = 0x00,
	icsp_cmd_load_data                          = 0x02,
	icsp_cmd_read_data                          = 0x04,
	icsp_cmd_increment_address                  = 0x06,
	icsp_cmd_reset_address                      = 0x16,
	icsp_cmd_begin_programming                  = 0x08,
	icsp_cmd_begin_externally_timed_programming = 0x18,
	icsp_cmd_end_externally_timed_programming   = 0x0a,
	icsp_cmd_bulk_erase                         = 0x09,
	icsp_cmd_row_erase                          = 0x11
};

BOOL usb_tasks(void) {
	USBDeviceTasks();
	if (USBDeviceState < CONFIGURED_STATE || USBSuspendControl) return 0;
	CDCTxService();
	return 1;
}

void icsp_bit(char x) {
	if (x) LATC |= 1;
	else   LATC &= ~1;
	LATC |= 1 << 1;
	LATC &= ~(1 << 1);
}

void icsp_begin() {
	TRISC = ~7; // Use RC0 RC1 and RC2 as outputs
	LATC = 0;
	for (size_t i = 0; i < 32; ++i) icsp_bit(0x4D434850 >> i & 1);
	icsp_bit(0);
}

void icsp_end() {
	TRISC = ~0;
}

void icsp_cmd(char c) {
	for (size_t i = 0; i < 6; ++i) {
		icsp_bit(c & 1);
		c >>= 1;
	}
}

void icsp_parameter(unsigned int p) {
	p <<= 1;
	for (size_t i = 0; i < 16; ++i) {
		icsp_bit(p & 1);
		p >>= 1;
	}
}

unsigned int icsp_read() {
	TRISC |= 1;
	unsigned int result = 0;
	for (size_t i = 0; i < 16; ++i) {
		LATC |= 1 << 1;
		LATC &= ~(1 << 1);
		result |= (PORTC & 1) << i;
	}
	TRISC &= ~1;
	return (result >> 1) & 0x3FFF;
}

BOOL read_value(unsigned int * value) {
	char a, b;
	while (!getsUSBUSART(&a, 1)) if (!usb_tasks()) return 0;
	if (!(a & 0x80)) return 0;
	while (!getsUSBUSART(&b, 1)) if (!usb_tasks()) return 0;
	if (!(b & 0x80)) return 0;
	*value = (a & 0x7F) << 7 | (b & 0x7F);
	return 1;
}

void write_value(unsigned int v) {
	char x[2];
	x[0] = 0x80 | (v >> 7);
	x[1] = 0x80 | v;
	putUSBUSART(x, 2);
}

char version[] = "PIC16F145x programmer 1.1 by Mara Bos <m-ou.se@m-ou.se>\n";

int main(void) {
	OSCTUNE = 0;
	OSCCON = 0xFC; // 16MHz HFINTOSC with 3x PLL enabled (48MHz operation)
	ACTCON = 0x90; // Enable active clock tuning with USB
	TRISA = 0xFF;
	TRISC = 0xFF;
	ANSELC = 0x00;
	OPTION_REG &= 0x7F; // WPUEN
	WPUA = 1 << 5;
	USBDeviceInit();

	while (1) {
		while (!usb_tasks());
		char cmd;
		if (getsUSBUSART(&cmd, 1)) {
			     if (cmd == 'V') putUSBUSART(version, sizeof(version) - 1);
			else if (cmd == 'T') { char r = 'Y'; putUSBUSART(&r, 1); }
			else if (cmd == 'B') icsp_begin();
			else if (cmd == 'E') icsp_end();
			else if (cmd == 'C') { unsigned int value; if (read_value(&value)) { icsp_cmd(icsp_cmd_load_configuration); icsp_parameter(value); } }
			else if (cmd == 'L') { unsigned int value; if (read_value(&value)) { icsp_cmd(icsp_cmd_load_data); icsp_parameter(value); } }
			else if (cmd == 'R') { icsp_cmd(icsp_cmd_read_data); write_value(icsp_read()); }
			else if (cmd == 'I') icsp_cmd(icsp_cmd_increment_address);
			else if (cmd == 'A') icsp_cmd(icsp_cmd_reset_address);
			else if (cmd == 'P') icsp_cmd(icsp_cmd_begin_programming);
			else if (cmd == 'Q') icsp_cmd(icsp_cmd_begin_externally_timed_programming);
			else if (cmd == 'S') icsp_cmd(icsp_cmd_end_externally_timed_programming);
			else if (cmd == 'X') icsp_cmd(icsp_cmd_bulk_erase);
			else if (cmd == 'Y') icsp_cmd(icsp_cmd_row_erase);
		}
	}

}

BOOL USER_USB_CALLBACK_EVENT_HANDLER(int event, void *, WORD) {
	switch (event) {
		case EVENT_CONFIGURED:
			CDCInitEP();
			break;
		case EVENT_EP0_REQUEST:
			USBCheckCDCRequest();
			break;
		case EVENT_TRANSFER:
		case EVENT_SOF:
		case EVENT_SUSPEND:
		case EVENT_RESUME:
		case EVENT_SET_DESCRIPTOR:
		case EVENT_BUS_ERROR:
		case EVENT_TRANSFER_TERMINATED:
		default:
			break;
	}
	return TRUE;
}

