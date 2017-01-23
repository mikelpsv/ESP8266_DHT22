#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "user_interface.h"
#include "gpio.h"
#include "gpio_util.h"
#include "driver/dht.h"


bool dht_read(dht_sensor *sensor, dht_data* output)
{
	int counter = 0;
	int laststate = 1;
	int i = 0;
	int j = 0;
	int checksum = 0;
	int data[100];
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	uint8_t pin = pin_num[sensor->pin];

	// Wake up device, 250ms of high
	GPIO_OUTPUT_SET(pin, 1);
	os_delay_us(250*1000);
	
	// Hold low for 20ms
	GPIO_OUTPUT_SET(pin, 0);
	os_delay_us(20*1000);

	// High for 40ns
	GPIO_OUTPUT_SET(pin, 1);
	os_delay_us(40);

	// Set DHT_PIN pin as an input
	GPIO_DIS_OUTPUT(pin);

	// wait for pin to drop?
	while (GPIO_INPUT_GET(pin) == 1 && i < DHT_MAXCOUNT) {
		os_delay_us(1);
		i++;
	}

	if(i == DHT_MAXCOUNT)
	{
	    return false;
	}

	// read data
	for (i = 0; i < DHT_MAXTIMINGS; i++)
	{
		// Count high time (in approx us)
		counter = 0;
		while (GPIO_INPUT_GET(pin) == laststate)
		{
			counter++;
			os_delay_us(1);
			if (counter == 1000)
				break;
		}
		laststate = GPIO_INPUT_GET(pin);
		if (counter == 1000)
			break;
		// store data after 3 reads
		if ((i>3) && (i%2 == 0)) {
			// shove each bit into the storage bytes
			data[j/8] <<= 1;
			if (counter > DHT_BREAKTIME)
				data[j/8] |= 1;
			j++;
		}
	}

	if (j >= 39) {
		checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
//	    DHT_DEBUG("DHT%s: %02x %02x %02x %02x [%02x] CS: %02x (GPIO%d)\r\n",
//	              sensor->type==DHT11?"11":"22",
//	              data[0], data[1], data[2], data[3], data[4], checksum, pin);
		if (data[4] == checksum) {
			// checksum is valid
			output->temperature = scale_temperature(sensor->type, data);
			output->humidity = scale_humidity(sensor->type, data);
			//DHT_DEBUG("DHT: Temperature =  %d *C, Humidity = %d %%\r\n", (int)(reading.temperature * 100), (int)(reading.humidity * 100));
//			DHT_DEBUG("DHT: Temperature*100 =  %d *C, Humidity*100 = %d %% (GPIO%d)\n",
//		          (int) (output->temperature * 100), (int) (output->humidity * 100), pin);
		} else {
			//DHT_DEBUG("Checksum was incorrect after %d bits. Expected %d but got %d\r\n", j, data[4], checksum);
//			DHT_DEBUG("DHT: Checksum was incorrect after %d bits. Expected %d but got %d (GPIO%d)\r\n",
//		                j, data[4], checksum, pin);
		    return false;
		}
	} else {
		//DHT_DEBUG("Got too few bits: %d should be at least 40\r\n", j);
//	    DHT_DEBUG("DHT: Got too few bits: %d should be at least 40 (GPIO%d)\r\n", j, pin);
	    return false;
	}
	return true;
}

bool dht_init(dht_sensor *sensor)
{
	if (set_gpio_mode(sensor->pin, GPIO_PULLUP, GPIO_INPUT)) {
		return true;
	} else {
		return false;
	}
}