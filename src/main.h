// Width and height of the eInk display
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480
// MQTT topics for sleep time and the image URL
#define SLEEP_TIME_TOPIC "what-to-wear/nextUpdateIn"
#define IMAGE_URL_TOPIC "what-to-wear/rawImageURL"
// Fallback sleep time when invalid data is provided through MQTT [ms]
#define SLEEP_TIME_DEFAULT 5 * 60 * 1000
// Minimum interval between retries to update the image [ms]
#define MIN_CHECK_INTERVAL 10 * 1000
// Maximum number of retries before going to sleep and trying again later
#define MAX_RETRIES 3

// Hardware pin configuration
#define RST_PIN     16
#define DC_PIN      17
#define CS_PIN      SS
#define BUSY_PIN    4

// Watchdog timeout (in us) for reset in case the software hangs
#define WATCHDOG_TIMEOUT 10000000