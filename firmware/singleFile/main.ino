#define UPDATE_FREQUENCY 60000 

unsigned int lastPublish;

void setup() {
    Serial.begin(115200);
  
    Particle.subscribe("hook-response/wifi_scan", onLocationReceived, MY_DEVICES);
}

void loop() {
    unsigned int now = millis();
    if ((now - lastPublish) > UPDATE_FREQUENCY) {
        WifiScan();
        lastPublish = now;
    }
    
    delay(10);
}

void WifiScan() {
    Serial.println("starting scan");
    
    int count = 0;
    String wifiList = "{\"list\":[";
    
    
    WiFiAccessPoint aps[20];
    int found = WiFi.scan(aps, 20);
    for (int i=0; i<found; i++) {
        WiFiAccessPoint& ap = aps[i];
        
        //{ "macAddress": "01:23:45:67:89:ab", "signalStrength": -51 }, 
        
        String line = String::format("{\"m\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"s\":%d}",
            ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5],
            ap.rssi
        );
        
        Serial.println(line);
        
        if (count < 5) {
            if (count > 0) {
                wifiList += ",";
            }
            
            wifiList += line;
            count++;
        }
        
        
    }
    wifiList += "]}";
    
    if (count == 5) {
        // for simplicity right now, lets only do it if we see at least 5 APs.
        Particle.publish("wifi_scan", wifiList, PRIVATE);
        
        // looks something like this
        //{"name":"wifi_scan",
        //"data":"{\"list\":[{\"m\":\"70:5A:0F:F8:51:49\",\"s\":-60},{\"m\":\"6C:3B:E5:4E:7A:58\",\"s\":-77},{\"m\":\"52:6D:64:28:A2:C0\",\"s\":-70},{\"m\":\"B4:C7:99:B1:2F:F8\",\"s\":-65},{\"m\":\"B4:C7:99:B1:2F:F0\",\"s\":-66}]}",
        //"ttl":"60",
        //"published_at":"2016-09-22T21:56:48.130Z",
        //"coreid":"abc123"}
    }
}


// data should be "lat,lng,accuracy"
void onLocationReceived(const char *event, const char *data) {
    if (strcmp(data, "{}") == 0) {
        Serial.println("got a contribute link back");
        return;
    }
    
    Serial.println("got location back " + String(data));
    
    String str = String(data);
    String one, two, three;
    //char one[32], two[32], three[32];
    
    double latitude, longitude, accuracy;
    //sscanf(data, "%*s:%*s:%d,%*s:%d,%*s:%d", &latitude, &longitude, &accuracy);
    //sscanf(data, "%[^,],%[^,],%[^,]", one, two, three);
    one = tryExtractString(0, str, "lat\":", ",");
    two = tryExtractString(0, str, "lng\":", "}");
    three = tryExtractString(0, str, "accuracy\":", "}");
/*    Serial.println(String(one));
    Serial.println(String(two));
    Serial.println(String(three));
*/    
    latitude = String(one).toFloat();
    longitude = String(two).toFloat();
    accuracy = String(three).toFloat();
    
    if (latitude ==  0) {
        Serial.println("nevermind");
        return;
    }
    
    
    Serial.println(String::format("Parsed into %f, %f, %f ", latitude, longitude, accuracy));
    Serial.println(String::format("You're on a map! https://www.google.com/maps/place/%f,%f", latitude, longitude));
    
    String dataJson = String("{")
        + "\"c_lat\":" + String::format("%f", latitude)
        + ",\"c_lng\":" + String::format("%f", longitude)
        + ",\"c_unc\":" + String::format("%f", accuracy)
        + "}";
        
    Particle.publish("trk/loc", dataJson, 60, PRIVATE);
}

String tryExtractString(int matchNum, String str, const char* start, const char* end) {
    if (str == NULL) {
        return NULL;
    }

    int count = 0;
    int lastIdx = 0;

    while (count <= matchNum) {
        int idx = str.indexOf(start, lastIdx);

        int endIdx = str.indexOf(end, lastIdx + idx + strlen(start));

        lastIdx = endIdx;

        if (count == matchNum) {
            return str.substring(idx + strlen(start), endIdx);
        }
        count++;
    }
}

// For getting our location back
//
// https://location.services.mozilla.com/v1/geolocate?key=<API_KEY>
// {
//     "wifiAccessPoints": [
//         { "macAddress": "01:23:45:67:89:ab", "signalStrength": -51 }, 
//         { "macAddress": "01:23:45:67:89:cd" }
//     ]
// }


// for sending data to Mozilla
//
// https://location.services.mozilla.com/v2/geosubmit?key=<API_KEY>
// {
//     "items": [{
//         "timestamp": 1405602028568,
//         "wifiAccessPoints": [
//             {
//                 "macAddress": "01:23:45:67:89:ab",
//                 "age": 5000,
//                 "channel": 6,
//                 "frequency": 2437,
//                 "radioType": "802.11n",
//                 "signalToNoiseRatio": 13,
//                 "signalStrength": -77
//             },
//             {
//                 "macAddress": "23:45:67:89:ab:cd"
//             }
//         ]
// }]}

