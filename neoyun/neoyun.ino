/**
 * API set value format: http://<address>/arduino/<key>/<value>
 * API get value format: http://<address>/arduino/<key>
 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define NEOCTRLPIN      6
#define NEOPIXELS       8
#define POLLTMOUT      50
#define PIN_STATUS     13

#define LENGTH(x)      (sizeof(x)/sizeof(*x))

#define LED_COLOR_GREEN 0x00FF00
#define LED_COLOR_RED   0xFF0000

const struct entry {
	const String key;
	const unsigned pin;
	const uint32_t color;
} collection[] = {
	{ "pl_chertsey",         0, LED_COLOR_RED   },
	{ "pl_hertfordshire",    1, LED_COLOR_GREEN },
	{ "pl_kits_coty_house",  2, LED_COLOR_RED   },
	{ "pl_northumbeland",    3, LED_COLOR_GREEN },
	{ "pl_kent",             4, LED_COLOR_GREEN },
	{ "pl_cornwall",         5, LED_COLOR_GREEN },
	{ "pl_isle_of_anglesey", 6, LED_COLOR_GREEN },
	{ "pl_stonehenge",       7, LED_COLOR_RED   },
};

YunServer server = YunServer();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOPIXELS, NEOCTRLPIN, NEO_GRB + NEO_KHZ800);

void setup(void) {
#if defined (__AVR_ATtiny85__)
	if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
	pinMode(13, OUTPUT);
	Bridge.begin();
	server.listenOnLocalhost();
	server.begin();
	strip.begin();
	strip_init();
}

void strip_init(void) {
	for (size_t i = 0; i < NEOPIXELS; ++i) {
		strip.setPixelColor(i, LOW);
	}
	strip.show();
}

void loop(void) {
	YunClient client = server.accept();
	if (client) {
		digitalWrite(PIN_STATUS, HIGH);
		process(client);
		client.stop();
		digitalWrite(PIN_STATUS, LOW);
	}
	delay(POLLTMOUT);
}

void process(YunClient client) {
	String key = "";
	String val = "";

	String rest = client.readString();
	rest.trim();

	int i = rest.indexOf('/');
	if (i < 0) {
		key = rest;
	} else {
		key = rest.substring(0, i);
		val = rest.substring(i+1);
	}

	const struct entry *e = lookupEntry(key);
	if (e && val.length()) {
		const long value = val.toInt();
		strip.setPixelColor(e->pin, value ? e->color : LOW);
		strip.show();
	}

	feedback(client, e);
}

const struct entry* lookupEntry(const String key) {
	for (size_t i = 0; i < LENGTH(collection); ++i) {
		const struct entry *e = &collection[i];
		if (e->key == key) {
			return e;
		}
	}
	return NULL;
}

void feedback(YunClient client, const struct entry *e) {
	static const String state[] = { "off", "on" };

	if (!e) {
		client.println(F("invalid key"));
		return;
	}

	const uint32_t value = strip.getPixelColor(e->pin);

	client.print(e->key);
	client.print(F("::"));
	client.print(e->pin);
	client.print(F(" is "));
	client.print(state[!!value]);
	client.print(F("::"));
	client.print(value);
	client.println();
}

