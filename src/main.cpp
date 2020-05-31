#include "main.h"

#include <HTTPClient.h>
#include <Homie.h>
#include <SPI.h>
#include <WiFiClient.h>

#include <string>

#include "mbedtls/md.h"
#include "version.h"
#include "waveshare/epd7in5_V2.h"

unsigned char IMAGE_BUFFER[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8]{0x00};
uint32_t lastUpdate = -1;  // in ms
uint32_t sleepTime = 0;    // in ms
String imageURL;
uint8_t retries;

// Hashing context and computed hash of the currently displayed image
mbedtls_md_context_t ctx;
mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
byte lastHash[32]{0x00};

HTTPClient c;
AsyncMqttClient mqttClient;
Epd epd;

/* updateImage() will attempt do download and display the image.
 * It returns 0 if the operation is deemed successful.
 */
uint8_t updateImage() {
  if (imageURL.isEmpty()) {
    return -1;
  }
  HTTPClient http;
  http.begin(imageURL);
  int httpCode = http.GET();
  Homie.getLogger() << "Got return code: " << httpCode << endl;
  if (httpCode == 200) {
    String data = http.getString();
    http.end();
    Homie.getLogger() << "Received image data with a length of: "
                      << data.length() << endl;
    if (data.length() > 0) {
      // Calculate checksum of the newly received image data
      byte currentHash[32];
      mbedtls_md_init(&ctx);
      mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
      mbedtls_md_starts(&ctx);
      mbedtls_md_update(&ctx, (const unsigned char *)data.c_str(),
                        data.length());
      mbedtls_md_finish(&ctx, currentHash);
      mbedtls_md_free(&ctx);

      // Compare checksums
      bool identical = true;
      for (int i = 0; i < 32; i++) {
        if (lastHash[i] != currentHash[i]) {
          identical = false;
          break;
        }
      }

      // Only update if the images are actually different
      if (!identical) {
        // Copy data to image buffer
        for (int i = 0; i < data.length(); i++) {
          IMAGE_BUFFER[i] = data[i];
        }

        // Initialize the display
        Homie.getLogger() << "Initializing display..." << endl;
        // Reset and delay seem to be required prior to epd.Init()...
        epd.Reset();
        delay(100);
        if (epd.Init() != 0) {
          Homie.getLogger() << "Display initialization failed." << endl;
          return -1;
        }

        // Update and sleep
        Homie.getLogger() << "Displaying image..." << endl;
        epd.DisplayFrame((const unsigned char *)IMAGE_BUFFER);
        epd.Sleep();

        // Copy the hash when everything was successfull
        for (int i = 0; i < 32; i++) {
          lastHash[i] = currentHash[i];
        }
        return 0;
      } else {
        Homie.getLogger() << "Image is identical - not updating." << endl;
        return 0;
      }
    } else {
      return -1;
    }
  } else {
    return -1;
  }
}

void onMqttMessage(char *topic, char *payload,
                   AsyncMqttClientMessageProperties properties, size_t len,
                   size_t index, size_t total) {
  if (strcmp(SLEEP_TIME_TOPIC, topic) == 0) {
    char tmp[len + 1];
    strncpy(tmp, payload, len);
    tmp[len] = '\0';
    Homie.getLogger() << "Received sleep time update: " << tmp << endl;
    int i = atoi(tmp);
    if (i > 0) {
      sleepTime = 1000 * i;
    } else {  // If we can't parse sleep time, fall back to a sane default
      sleepTime = SLEEP_TIME_DEFAULT;
    }
  } else if (strcmp(IMAGE_URL_TOPIC, topic) == 0) {
    char tmp[len + 1];
    strncpy(tmp, payload, len);
    tmp[len] = '\0';
    Homie.getLogger() << "Received image URL: " << tmp << endl;
    
    imageURL = String(tmp);
  }
}

void onHomieEvent(const HomieEvent &event) {
  switch (event.type) {
    case HomieEventType::STANDALONE_MODE:
      break;
    case HomieEventType::CONFIGURATION_MODE:
      break;
    case HomieEventType::NORMAL_MODE:
      break;
    case HomieEventType::OTA_STARTED:
      break;
    case HomieEventType::OTA_PROGRESS:
      break;
    case HomieEventType::OTA_FAILED:
      break;
    case HomieEventType::OTA_SUCCESSFUL:
      break;
    case HomieEventType::ABOUT_TO_RESET:
      break;
    case HomieEventType::WIFI_CONNECTED:
      break;
    case HomieEventType::WIFI_DISCONNECTED:
      break;
    case HomieEventType::MQTT_READY:
      mqttClient = Homie.getMqttClient();
      mqttClient.onMessage(onMqttMessage);
      Homie.getLogger() << "Subscribing to MQTT topics" << endl;
      mqttClient.subscribe(SLEEP_TIME_TOPIC, 1);
      mqttClient.subscribe(IMAGE_URL_TOPIC, 1);
      break;
    case HomieEventType::MQTT_DISCONNECTED:
      break;
    case HomieEventType::MQTT_PACKET_ACKNOWLEDGED:
      break;
    case HomieEventType::READY_TO_SLEEP:
      lastUpdate = -1;
      Homie.doDeepSleep(1000 * sleepTime);
      break;
    case HomieEventType::SENDING_STATISTICS:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Homie_setFirmware(FW_NAME, FW_VERSION);
  Homie.onEvent(onHomieEvent);
  Homie.setup();
}

void loop() {
  Homie.loop();

  /* As soon as Homie is connected, we'll watch if we received MQTT
   * configuration data. If so, we'll try to update the image and got to sleep
   * afterwards.
   */
  if (Homie.isConnected() && sleepTime != -1 && !imageURL.isEmpty()) {
    // Rate limit the update
    auto currentTime = millis();
    if (lastUpdate == -1 || currentTime > (lastUpdate + MIN_CHECK_INTERVAL)) {
      uint8_t ret = updateImage();
      if(ret == 0) {
        retries = 0;
        Homie.prepareToSleep();
      } else {
        retries++;
        if(retries >= MAX_RETRIES) {
          Homie.getLogger() << "Exceeded number of retries - giving up." << endl;
          retries = 0;
          Homie.prepareToSleep();
        }
      }

      lastUpdate = currentTime;
    }
  }
}
