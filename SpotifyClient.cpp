

#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_VERBOSE

#include <HTTPClient.h>
#include "SpotifyClient.h"
#include <base64.h>


SpotifyClient::SpotifyClient(String clientId, String clientSecret, String deviceName, String refreshToken) {
  this->clientId = clientId;
  this->clientSecret = clientSecret;
  this->refreshToken = refreshToken;
  this->deviceName = deviceName;

  client.setCACert(digicert_root_ca);
  //client.setCACert(amazon_root_ca);
  
}

void SpotifyClient::FetchToken()
{
    HTTPClient http;

    String body = "grant_type=refresh_token&refresh_token=" + refreshToken;
    String authorizationRaw = clientId + ":" + clientSecret;
    String authorization =  base64::encode(authorizationRaw);
    http.begin(client, "https://accounts.spotify.com/api/token"); 
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Basic " + authorization);

    int httpCode = http.POST(body);                                   
    if (httpCode > 0) { //Check for the returning code
        String returnedPayload = http.getString();
        if ( httpCode == 200 )
        {
            accessToken = ParseJson("access_token", returnedPayload );
            Serial.println("Got new access token");
        }
        else
        {
            Serial.println("Failed to get new access token");
            Serial.println(httpCode);
            Serial.println(returnedPayload);
        }
    }
    else {
        Serial.println("Failed to connect to https://accounts.spotify.com/api/token" );
    }
    http.end(); //Free the resources
}

int SpotifyClient::Shuffle()
{
    Serial.println("Shuffle()");
    HttpResult result = CallAPI( "PUT", "https://api.spotify.com/v1/me/player/shuffle?state=true&device_id=" + deviceId, "" );
    //int httpCode = CallAPI( "PUT", "https://api.spotify.com/v1/me/player/shuffle?state=true&device_id=" + deviceId, "" );
    return result.httpCode;
}

int SpotifyClient::Next()
{
    Serial.println("Next()");
    HttpResult result = CallAPI( "POST", "https://api.spotify.com/v1/me/player/next?device_id=" + deviceId, "" );
    //int httpCode = CallAPI( "POST", "https://api.spotify.com/v1/me/player/next?device_id=" + deviceId, "" );
    return result.httpCode;
}


int SpotifyClient::Play(String context_uri )
{
    Serial.println("Play()");
    String body = "{\"context_uri\":\"" + context_uri + "\",\"offset\":{\"position\":0,\"position_ms\":0}}";
    String url = "https://api.spotify.com/v1/me/player/play?device_id=" + deviceId;

    HttpResult result = CallAPI( "PUT", url, body );
    //int httpCode = CallAPI( "PUT", url, body );

/*
    HTTPClient http;
    http.begin(client, "https://api.spotify.com/v1/me/player/play?device_id=" + deviceId); 

    
    String authorization = "Bearer " + accessToken;

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", authorization);

    int httpCode = http.PUT(body);                                   
    if (httpCode > 0) { //Check for the returning code
        Serial.println(httpCode);
    }
    else {
        Serial.println("Error on HTTP request httpCode:" );
    }
    http.end(); //Free the resources
*/
    return result.httpCode;
}

HttpResult SpotifyClient::CallAPI( String method, String url, String body )
{
    HttpResult result;
    result.httpCode = 0;
    Serial.print(url);
    Serial.print( " returned: " );

    HTTPClient http;
//    http.begin(client, "https://api.spotify.com/v1/me/player/play?device_id=" + deviceId); 
    http.begin(client, url); 
    
    String authorization = "Bearer " + accessToken;

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", authorization);

    if ( body.length() == 0 )
    {
         http.addHeader(F("Content-Length"), String(0));
    }

    //int httpCode = http.PUT(body);                                 
    //int httpCode = 0;
    if ( method == "PUT" )
    {
        result.httpCode = http.PUT(body); 
    }
    else if ( method == "POST" )
    {
        result.httpCode = http.POST(body); 
    }
    else if ( method == "GET" )
    {
        result.httpCode = http.GET(); 
    }    

    if (result.httpCode > 0) { //Check for the returning code
        Serial.println(result.httpCode);
        if ( http.getSize() > 0 ){
            result.payload = http.getString();
        }
        
    }
    else {
        Serial.print("Failed to connect to ");
        Serial.println(url);
    }
    http.end(); //Free the resources

    return result;
}

/*
int SpotifyClient::CallAPI( String method, String url, String body )
{
    HTTPClient http;
    http.begin(client, url); 
    http.addHeader("Content-Type", "application/json");
    String authorization = "Bearer " + accessToken;
    http.addHeader("Authorization", authorization);

    int httpCode = 0;
    if ( method == "PUT" )
    {
        httpCode = http.PUT(body); 
    }
    else if ( method == "POST" )
    {
        httpCode = http.POST(body); 
    }
    else if ( method == "GET" )
    {
        httpCode = http.GET(); 
    }
    if (httpCode > 0) { //Check for the returning code
        String returnedPayload = http.getString();
        if ( httpCode == 200 )
        {
            accessToken = ParseJson("access_token", returnedPayload );
            Serial.println("Got new access token");
        }
        else
        {
            Serial.print("API call returned error: ");
            Serial.println(httpCode);
            Serial.println(returnedPayload);
        }
    }
    else {
        Serial.print("Failed to connect to ");
        Serial.println(url);
    }
    http.end(); //Free the resources

    return httpCode;
}
*/

String SpotifyClient::GetDevices()
{
     HttpResult result = CallAPI( "GET", "https://api.spotify.com/v1/me/player/devices", "" );
     deviceId = GetDeviceId(result.payload);
     Serial.print("Device ID: ");
     Serial.println(deviceId);
}


String SpotifyClient::GetDeviceId(String json)
{
    String id = "";

    // find the position the device name
    int index = json.indexOf(deviceName);
    if ( index > 0 )
    {
        // we found it, so not backup to begining of this object
        int i = index;
        for (; i > 0; i--  )
        {
            if ( json.charAt(i) == '{')
            {
                break;
            }
        }

        // now move forward and find "id"
        for (; i < json.length(); i++ )
        {
            if ( json.charAt(i) == '}')
            {
                break;
            }

            if ( i + 3 < json.length() && json.charAt(i) == '"' 
            && json.charAt(i+1) == 'i' && json.charAt(i+2) == 'd' && json.charAt(i+3) == '"')
            {
                i += 4;
                break;
            }
        }

        // move forward to next "
        for (; i < json.length() ; i++ ){
            if ( json.charAt(i) == '"'){
                i++;
                break;
            }
        }

        // now get that device id
        for (; i < json.length() ; i++ ){
            if ( json.charAt(i) != '"' )
            {
                id += json.charAt(i);
            }
            else
            {
                break;
            }
            
        }
    }
    else
    {
        Serial.print(deviceName);
        Serial.println( " device name not found.");
    }
    return id;
}

String SpotifyClient::ParseJson(String key, String json )
{
    String retVal = "";    
    int index = json.indexOf(key);

     //Serial.print( "index: " );
     //Serial.println(index);
    if ( index > 0 )
    {
        bool copy = false;
        for ( int i = index; i < json.length(); i++ )
        {
            //Serial.print( "charAt: " );
            //Serial.println( json.charAt(i) );
            if ( copy )
            {
                if ( json.charAt(i) == '"' || json.charAt(i) == ',')
                {
                    break;
                }
                else
                {
                    retVal += json.charAt(i);
                }
            }
            else if ( json.charAt(i) == ':')
            {
                copy = true;
                if ( json.charAt(i+1) == '"' )
                {
                    i++;
                }
            }
        }
    }
    return retVal;
}