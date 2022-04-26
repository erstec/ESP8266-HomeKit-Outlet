#include <Arduino.h>

#include <settings.h>

#include <ESP8266WiFi.h>

#include "WiFiManager.h"

#if defined(USE_SPIFFS)
//#include <FS.h>
#include <LittleFS.h>
#endif

// #include <ArduinoOTA.h>
#include "ESP8266WebServer.h"
#include "ESP8266HTTPUpdateServer.h"
#if defined(HTTPS)
#include "ESP8266WebServerSecure.h"
#endif

#include <arduino_homekit_server.h>
#include "ButtonDebounce.h"
#include "ButtonHandler.h"

ButtonDebounce btn(PIN_BUTTON, INPUT_PULLUP, LOW);
ButtonHandler btnHandler(10000);

void accessory_init();
void switchToggle();
void builtinledSetStatus(bool on);
void homekit_setup();
void homekit_loop();
void my_homekit_report();

bool runOnce = false;
bool switch_power = false;

//const char *ssid = "SilentNight-2G";
//const char *password = "HarisPoteris2012!!";

#define SIMPLE_INFO(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

void IRAM_ATTR btnInterrupt() {
	btn.update();
}

void blink_led(int interval, int count) {
	for (int i = 0; i < count; i++) {
		builtinledSetStatus(true);
		delay(interval);
		builtinledSetStatus(false);
		delay(interval);
	}
}

#if defined(HTTPS)
ESP8266WebServer httpServer80(80);
ESP8266WebServerSecure httpServer(443);
ESP8266HTTPUpdateServerSecure httpUpdater;
#else
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
#endif

#if defined(HTTPS)
static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDSzCCAjMCCQD2ahcfZAwXxDANBgkqhkiG9w0BAQsFADCBiTELMAkGA1UEBhMC
VVMxEzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDU9yYW5nZSBDb3VudHkx
EDAOBgNVBAoMB1ByaXZhZG8xGjAYBgNVBAMMEXNlcnZlci56bGFiZWwuY29tMR8w
HQYJKoZIhvcNAQkBFhBlYXJsZUB6bGFiZWwuY29tMB4XDTE4MDMwNjA1NDg0NFoX
DTE5MDMwNjA1NDg0NFowRTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3Rh
dGUxITAfBgNVBAoMGEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDCCASIwDQYJKoZI
hvcNAQEBBQADggEPADCCAQoCggEBAPVKBwbZ+KDSl40YCDkP6y8Sv4iNGvEOZg8Y
X7sGvf/xZH7UiCBWPFIRpNmDSaZ3yjsmFqm6sLiYSGSdrBCFqdt9NTp2r7hga6Sj
oASSZY4B9pf+GblDy5m10KDx90BFKXdPMCLT+o76Nx9PpCvw13A848wHNG3bpBgI
t+w/vJCX3bkRn8yEYAU6GdMbYe7v446hX3kY5UmgeJFr9xz1kq6AzYrMt/UHhNzO
S+QckJaY0OGWvmTNspY3xCbbFtIDkCdBS8CZAw+itnofvnWWKQEXlt6otPh5njwy
+O1t/Q+Z7OMDYQaH02IQx3188/kW3FzOY32knER1uzjmRO+jhA8CAwEAATANBgkq
hkiG9w0BAQsFAAOCAQEAnDrROGRETB0woIcI1+acY1yRq4yAcH2/hdq2MoM+DCyM
E8CJaOznGR9ND0ImWpTZqomHOUkOBpvu7u315blQZcLbL1LfHJGRTCHVhvVrcyEb
fWTnRtAQdlirUm/obwXIitoz64VSbIVzcqqfg9C6ZREB9JbEX98/9Wp2gVY+31oC
JfUvYadSYxh3nblvA4OL+iEZiW8NE3hbW6WPXxvS7Euge0uWMPc4uEcnsE0ZVG3m
+TGimzSdeWDvGBRWZHXczC2zD4aoE5vrl+GD2i++c6yjL/otHfYyUpzUfbI2hMAA
5tAF1D5vAAwA8nfPysumlLsIjohJZo4lgnhB++AlOg==
-----END CERTIFICATE-----
)EOF";

static const char serverKey[] PROGMEM =  R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEA9UoHBtn4oNKXjRgIOQ/rLxK/iI0a8Q5mDxhfuwa9//FkftSI
IFY8UhGk2YNJpnfKOyYWqbqwuJhIZJ2sEIWp2301OnavuGBrpKOgBJJljgH2l/4Z
uUPLmbXQoPH3QEUpd08wItP6jvo3H0+kK/DXcDzjzAc0bdukGAi37D+8kJfduRGf
zIRgBToZ0xth7u/jjqFfeRjlSaB4kWv3HPWSroDNisy39QeE3M5L5ByQlpjQ4Za+
ZM2yljfEJtsW0gOQJ0FLwJkDD6K2eh++dZYpAReW3qi0+HmePDL47W39D5ns4wNh
BofTYhDHfXzz+RbcXM5jfaScRHW7OOZE76OEDwIDAQABAoIBAQDKov5NFbNFQNR8
djcM1O7Is6dRaqiwLeH4ZH1pZ3d9QnFwKanPdQ5eCj9yhfhJMrr5xEyCqT0nMn7T
yEIGYDXjontfsf8WxWkH2TjvrfWBrHOIOx4LJEvFzyLsYxiMmtZXvy6YByD+Dw2M
q2GH/24rRdI2klkozIOyazluTXU8yOsSGxHr/aOa9/sZISgLmaGOOuKI/3Zqjdhr
eHeSqoQFt3xXa8jw01YubQUDw/4cv9rk2ytTdAoQUimiKtgtjsggpP1LTq4xcuqN
d4jWhTcnorWpbD2cVLxrEbnSR3VuBCJEZv5axg5ZPxLEnlcId8vMtvTRb5nzzszn
geYUWDPhAoGBAPyKVNqqwQl44oIeiuRM2FYenMt4voVaz3ExJX2JysrG0jtCPv+Y
84R6Cv3nfITz3EZDWp5sW3OwoGr77lF7Tv9tD6BptEmgBeuca3SHIdhG2MR+tLyx
/tkIAarxQcTGsZaSqra3gXOJCMz9h2P5dxpdU+0yeMmOEnAqgQ8qtNBfAoGBAPim
RAtnrd0WSlCgqVGYFCvDh1kD5QTNbZc+1PcBHbVV45EmJ2fLXnlDeplIZJdYxmzu
DMOxZBYgfeLY9exje00eZJNSj/csjJQqiRftrbvYY7m5njX1kM5K8x4HlynQTDkg
rtKO0YZJxxmjRTbFGMegh1SLlFLRIMtehNhOgipRAoGBAPnEEpJGCS9GGLfaX0HW
YqwiEK8Il12q57mqgsq7ag7NPwWOymHesxHV5mMh/Dw+NyBi4xAGWRh9mtrUmeqK
iyICik773Gxo0RIqnPgd4jJWN3N3YWeynzulOIkJnSNx5BforOCTc3uCD2s2YB5X
jx1LKoNQxLeLRN8cmpIWicf/AoGBANjRSsZTKwV9WWIDJoHyxav/vPb+8WYFp8lZ
zaRxQbGM6nn4NiZI7OF62N3uhWB/1c7IqTK/bVHqFTuJCrCNcsgld3gLZ2QWYaMV
kCPgaj1BjHw4AmB0+EcajfKilcqtSroJ6MfMJ6IclVOizkjbByeTsE4lxDmPCDSt
/9MKanBxAoGAY9xo741Pn9WUxDyRplww606ccdNf/ksHWNc/Y2B5SPwxxSnIq8nO
j01SmsCUYVFAgZVOTiiycakjYLzxlc6p8BxSVqy6LlJqn95N8OXoQ+bkwUux/ekg
gz5JWYhbD6c38khSzJb0pNXCo3EuYAVa36kDM96k1BtWuhRS10Q1VXk=
-----END RSA PRIVATE KEY-----
)EOF";
#endif

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t name;
extern "C" homekit_characteristic_t serial_number;
extern "C" homekit_characteristic_t cha_switch_on;

void handleRoot() {
	String s = "";

	s += 	"<!DOCTYPE HTML><html>" \
	     	"<head>" \
  			"	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" \
  			"	<style>" \
    		"		html {" \
     		"			font-family: Arial;" \
     		"			display: inline-block;" \
     		"			margin: 0px auto;" \
     		"			text-align: left;" \
    		"		}" \
    		"		h2 { font-size: 2.0rem; }" \
    		"		p { font-size: 1.0rem; }" \
    		"		.units { font-size: 1.2rem; }" \
  			"	</style>" \
			"</head>";
	s +=	"<body>" \
  			"	<h2>Arduino HomeKit Outlet v";
	s +=	VERSION;
	s +=	"</h2>" \
	  		"	<p>" \
    		"		<b>Name:</b>" \
    		"		<span id=\"system_name\">";
	s +=	name.value.string_value;
	s +=	"</span>" \
  			"	</p>" \
  			"	<p>" \
    		"		<b>Current Outlet State:</b>" \
    		"		<span id=\"outlet_state\">";
	s +=	switch_power ? "ON" : "OFF";
	s +=	"</span>" \
  			"	</p>" \
  			"	<p>" \
    		"		<b>S/N:</b>" \
    		"		<span id=\"serial_number\">";
	s +=	serial_number.value.string_value;
	s +=	"</span>" \
  			"	</p>" \
  			"	<p>" \
    		"		<b>MAC:</b>" \
    		"		<span id=\"mac_address\">";
	s +=	String(WiFi.macAddress());
	s +=	"</span>" \
  			"	</p>" \
  			"	<p>&nbsp;</p>" \
  			"	<p>- <strong>Short Press</strong> - Toggle ON/OFF</p>" \
  			"	<p>- <strong>Double Press</strong> - n/a</p>" \
  			"	<p>- <strong>Long Press (&gt;5sec)</strong> - Reset HomeKit pairing and WiFi settings.</p>" \
  			"	<p>- <strong>PowerUp with Button Pressed</strong> - Run WiFi Manager</p>" \
  			"	<p>&nbsp;</p>" \
  			"	<p><a href=\"/update\">Update FW</a></p>" \
			"</body>" \
			"</html>";
	
	httpServer.send(200, "text/html", s);
}

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void lowCPUspeed() {
	if (system_get_cpu_freq() != SYS_CPU_80MHZ) {
		system_update_cpu_freq(SYS_CPU_80MHZ);
		INFO("Update the CPU to run at 80 MHz");
	}
}

void setup() {
	Serial.begin(115200);
	Serial.setRxBufferSize(32);
	Serial.setDebugOutput(false);

#if defined(USE_RTC)
	uint32_t rtcMem;
	ESP.rtcUserMemoryRead(RTC_REGISTER_ADDRESS, &rtcMem, sizeof(rtcMem));
	// system_rtc_mem_read(56, &rtcMem, sizeof(rtcMem));
	Serial.print("\nBackup Register Read = ");
	Serial.println(rtcMem);
	switch_power = (rtcMem == 1);
	runOnce = true;
#endif

#if defined(USE_SPIFFS)
	if (LittleFS.begin()) {
		Serial.println("LitteFS begin OK");
	} else {
		Serial.println("LittleFS begin ERROR!");
	}
	File f = LittleFS.open("/state.bkp", "r");
	if (f) {
		Serial.println("File open read OK");
		String state = f.readString(); //read();
		Serial.println(state);

		// set last state here
		switch_power = (state == "ON");
		
		f.close();
	} else {
		Serial.println("File open read ERROR!");
	}
	runOnce = true;
#endif

#if !defined(USE_SPIFFS) && !defined(USE_RTC)
	pinMode(PIN_RELAY, OUTPUT);
	switch_power = (digitalRead(PIN_RELAY) == 1);
	runOnce = true;
#endif

	accessory_init();

	blink_led(50, 10);

	builtinledSetStatus(!switch_power);		// restore LED status after startup blinking

	btn.setCallback(std::bind(&ButtonHandler::handleChange, &btnHandler,
			std::placeholders::_1));
	btn.setInterrupt(btnInterrupt);
	btnHandler.setIsDownFunction(std::bind(&ButtonDebounce::checkIsDown, &btn));
	btnHandler.setCallback([](button_event e) {
		if (e == BUTTON_EVENT_SINGLECLICK) {
			SIMPLE_INFO("Button Event: SINGLECLICK");
			switchToggle();
		} else if (e == BUTTON_EVENT_DOUBLECLICK) {
			SIMPLE_INFO("Button Event: DOUBLECLICK");
		} else if (e == BUTTON_EVENT_LONGCLICK) {
			SIMPLE_INFO("Button Event: LONGCLICK");
			SIMPLE_INFO("Rebooting...");
			homekit_storage_reset();
			//WiFi.disconnect(true);
			ESP.eraseConfig();
			blink_led(50, 10);
			digitalWrite(PIN_LED, LOW);
			ESP.restart(); // or system_restart();
		}
	});

	WiFiManager wifiManager;
	wifiManager.setSaveConfigCallback(saveConfigCallback);
	wifiManager.setTimeout(300);
  	wifiManager.setConfigPortalTimeout(300);
	wifiManager.setShowPassword(true);

	//WiFi.mode(WIFI_STA);
	//WiFi.persistent(false);
	//WiFi.disconnect(false);
	//WiFi.setAutoReconnect(true);
	//WiFi.begin(ssid, password);

	pinMode(PIN_BUTTON, INPUT_PULLUP);
	if (LOW == digitalRead(PIN_BUTTON)) {
		wifiManager.startConfigPortal();
	}
	wifiManager.autoConnect();

	wifiManager.stopConfigPortal();
	WiFi.mode(WIFI_STA);

	WiFi.printDiag(Serial);

	SIMPLE_INFO("");
	SIMPLE_INFO("SketchSize: %d", ESP.getSketchSize());
	SIMPLE_INFO("FreeSketchSpace: %d", ESP.getFreeSketchSpace());
	SIMPLE_INFO("FlashChipSize: %d", ESP.getFlashChipSize());
	SIMPLE_INFO("FlashChipRealSize: %d", ESP.getFlashChipRealSize());
	SIMPLE_INFO("FlashChipSpeed: %d", ESP.getFlashChipSpeed());
	SIMPLE_INFO("SdkVersion: %s", ESP.getSdkVersion());
	SIMPLE_INFO("FullVersion: %s", ESP.getFullVersion().c_str());
	SIMPLE_INFO("CpuFreq: %dMHz", ESP.getCpuFreqMHz());
	SIMPLE_INFO("FreeHeap: %d", ESP.getFreeHeap());
	SIMPLE_INFO("ResetInfo: %s", ESP.getResetInfo().c_str());
	SIMPLE_INFO("ResetReason: %s", ESP.getResetReason().c_str());
	INFO_HEAP();

	homekit_setup();

	INFO_HEAP();

	blink_led(200, 3);
	
	builtinledSetStatus(!switch_power);	// restore LED status

	// Route for root / web page
	httpServer.on("/", HTTP_GET, handleRoot);

#if defined(HTTPS)
	httpServer.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
#endif

	httpUpdater.setup(&httpServer, "admin", "your_secret_password");
	//httpUpdater.setup(&httpServer);
	httpServer.begin();

#if defined(HTTPS)
	httpServer80.on("/", [](){
		httpServer80.sendHeader("Location", "/", true);
		//httpServer80.send(302, "text/plain", "https://");
		httpServer80.send(200, "text/plain", "Use https://");
	});
	httpServer80.begin();
#endif

	// lowCPUspeed();
}

void loop() {
	httpServer.handleClient();
#if defined(HTTPS)
	httpServer80.handleClient();
#endif
	homekit_loop();
}

void builtinledSetStatus(bool on) {
	digitalWrite(PIN_LED, on ? LOW : HIGH);
}

void saveCurrentState() {
#if defined(USE_SPIFFS)
	File f = LittleFS.open("/state.bkp", "w");
	if (f) {
		Serial.println("File open write OK");
		f.write(switch_power ? "ON" : "OFF");
		f.close();
	} else {
		Serial.println("File open write ERROR!");
	}
#endif

#if defined(USE_RTC)
	uint32_t rtcMem = (switch_power ? 1 : 0);
	ESP.rtcUserMemoryWrite(RTC_REGISTER_ADDRESS, &rtcMem, sizeof(rtcMem));
	Serial.print("Backup Register Write = ");
	Serial.println(rtcMem);
#endif
}

//==============================
// Homekit setup and loop
//==============================

void switchToggle() {
	switch_power = !switch_power;
	saveCurrentState();
	builtinledSetStatus(!switch_power);
	digitalWrite(PIN_RELAY, switch_power ? HIGH : LOW);
	cha_switch_on.value.bool_value = switch_power;
	homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void accessory_init() {
	Serial.println("Init accessory...");
	digitalWrite(PIN_RELAY, switch_power ? HIGH : LOW);
	pinMode(PIN_RELAY, OUTPUT);
	builtinledSetStatus(!switch_power);
	pinMode(PIN_LED, OUTPUT);
}

void cha_switch_on_setter(const homekit_value_t value) {
	printf("setter\n");
// if (value.format == homekit_format_bool) {
	bool on = value.bool_value;
	switch_power = on;
	saveCurrentState();
	// cha_switch_on.value.bool_value = on;	//sync the value
	Serial.printf("Switch: %s\n", on ? "ON" : "OFF");
	builtinledSetStatus(!switch_power);
	digitalWrite(PIN_RELAY, switch_power ? HIGH : LOW);
// }
}

homekit_value_t cha_switch_on_getter() {
	printf("getter\n");
	return HOMEKIT_BOOL_CPP(switch_power);
}

void homekit_setup() {
	uint8_t mac[WL_MAC_ADDR_LENGTH];
	WiFi.macAddress(mac);

	int name_len = snprintf(NULL, 0, "%s_%02X%02X%02X",
			name.value.string_value, mac[3], mac[4], mac[5]);
	char *name_value = (char*) malloc(name_len + 1);
	snprintf(name_value, name_len + 1, "%s_%02X%02X%02X",
			name.value.string_value, mac[3], mac[4], mac[5]);
	name.value = HOMEKIT_STRING_CPP(name_value);

	int serial_number_len = snprintf(NULL, 0, "%s_%02X%02X%02X",
			serial_number.value.string_value, mac[3], mac[4], mac[5]);
	char *serial_number_value = (char*) malloc(serial_number_len + 1);
	snprintf(serial_number_value, serial_number_len + 1, "%s_%02X%02X%02X",
			serial_number.value.string_value, mac[3], mac[4], mac[5]);
	serial_number.value = HOMEKIT_STRING_CPP(serial_number_value);

	cha_switch_on.setter = cha_switch_on_setter;
	cha_switch_on.getter = cha_switch_on_getter;

	arduino_homekit_setup(&config);
}

void homekit_loop() {
	if (runOnce) {
		runOnce = false;
#if defined(USE_SPIFFS) || defined(USE_RTC)
		cha_switch_on.value.bool_value = switch_power;
		homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
#endif
	}
	btnHandler.loop();
	arduino_homekit_loop();
	static uint32_t next_heap_millis = 0;
	uint32_t time = millis();
	if (time > next_heap_millis) {
		SIMPLE_INFO("heap: %d, sockets: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
		next_heap_millis = time + 5000;
	}
}
