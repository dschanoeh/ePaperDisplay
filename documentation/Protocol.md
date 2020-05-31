# ePaperDisplay Exchange Protocol

This document describes the exchange protocol used by the ePaperDisplay firmware to retrieve image data through the network.

## MQTT

The server must update the following topics. Messages posted to the topics must be retained to ensure asynchronous operation. When the ePaperDisplay awakes from a deep sleep, the broker will deliver the latest valid information.

### nextUpdateIn

The default topic path is: ```what-to-wear/nextUpdateIn```

The server must post an integer representing the estimated number of seconds till the next image update to this location.

This is used by the ePaperDisplay to schedule an appropriately long deep sleep to save energy.

### rawImageURL

The default topic path is: ```what-to-wear/rawImageURL```

The server must post a valid URL to this topic that points to the raw image data to be displayed.

## HTTP

The ePaperDisplay will issue a HTTP GET request to the _rawImageURL_ to retrieve the image data. The data returned by the server must be raw - other encodings (e.g. gzip) are not supported.

### Image Data

The image data itself is a stream of bits with each bit representing one pixel (no color or grey values supported). Pixels are counted from top-left to bottom-right. No control information is included.

For a display with a resolution of 800*480px, this should lead to an image data size of 800 * 480 / 8 = 48000 bytes.
