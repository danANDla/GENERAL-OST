#include "spw_layer.h"

int8_t spw_is_ready_to_transmit(const SWIC_SEND interface)
{
    return 1;
}

int8_t spw_send_data(
    const SWIC_SEND interface,
    const uint8_t *const data_buffer,
    const uint32_t data_size)
{
#if (defined(TARGET_MC30SF6))
    swic_send_packege(interface, data_buffer, data_size);
#elif ON_NS3
    Mac8Address addr;
    addr.CopyFrom(&to_address);
    spw_layer->Send(segment, addr, 0);
#endif
}

int8_t spw_hw_init()
{
	uint32_t i;
	for (i = 0; i <= TRY; i++)
	{
		swic_init(1);
		swic_init(0);
		delay(1000);
		if (swic_is_connected(1) == 1)
			break;
		debug_printf("SWIC 1 MODE: %x \n", GIGASPWR_COMM_SPW_MODE(1));
		debug_printf("SWIC 0 MODE: %x \n", GIGASPWR_COMM_SPW_MODE(0));
		debug_printf("SWIC 1 STATUS: %x \n", GIGASPWR_COMM_SPW_STATUS(1));
		debug_printf("SWIC 0 STATUS: %x \n", GIGASPWR_COMM_SPW_STATUS(0));
	}

	if (swic_is_connected(1) == 0)
	{
		debug_printf("OST SWIC CONNECTION FAIL! \n");
		return -1;
	}

	debug_printf("TRY SPEED_SET: %u \n", SWIC_SPEED);

	for (i = 0; i <= (TRY - 1); i++)
	{
		swic_set_tx_speed(0, SWIC_SPEED);
		swic_set_tx_speed(1, SWIC_SPEED);

		if (swic_is_connected(1) == 1)
			break;

		swic_init(1);
		swic_init(0);
		delay(1000);
		swic_set_tx_speed(0, SWIC_SPEED);
		swic_set_tx_speed(1, SWIC_SPEED);
	}

	if (swic_is_connected(1) == 0)
	{
		debug_printf("OST hardware Init FAIL! \n");
		return -1;
	}

	debug_printf("OST 0 RX_SPEED: %x \n", swic_get_rx_speed(0));
	debug_printf("OST 1 RX_SPEED: %x \n", swic_get_rx_speed(1));
}