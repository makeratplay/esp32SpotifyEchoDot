#include <Arduino.h>
#include <WiFi.h>
#include "SpotifyClient.h"

#include <SPI.h>
#include "MFRC522.h"

#include "settings.h"

#define RST_PIN 22 // Configurable, see typical pin layout above
#define SS_PIN 21  // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

byte const BUFFERSiZE = 176;


SpotifyClient spotify = SpotifyClient(clientId, clientSecret, deviceName, refreshToken);

void setup()
{
  Serial.begin(115200);
  Serial.println("Setup started");

  connectWifi();

  // Init SPI bus and MFRC522 for NFC reader
  SPI.begin();        
  mfrc522.PCD_Init(); 

  // Refresh Spotify Auth token and Deivce ID
  spotify.FetchToken();
  spotify.GetDevices();
}

void loop()
{
  if (mfrc522.PICC_IsNewCardPresent())
  {
    Serial.println("NFC tag present");
    readNFCTag();
  }
}

void readNFCTag()
{
  if (mfrc522.PICC_ReadCardSerial())
  {
    byte dataBuffer[BUFFERSiZE];
    readNFCTagData(dataBuffer);
    mfrc522.PICC_HaltA();

    //hexDump(dataBuffer);
    Serial.print("Read NFC tag: ");
    String context_uri = parseNFCTagData(dataBuffer);
    Serial.println(context_uri);
    playSpotifyUri(context_uri);
  }
}

void playSpotifyUri(String context_uri)
{
  int code = spotify.Play(context_uri);
  switch (code)
  {
    case 404:
    {
      // device id changed, get new one
      spotify.GetDevices();
      spotify.Play(context_uri);
      spotify.Shuffle();
      break;
    }
    case 401:
    {
      // auth token expired, get new one
      spotify.FetchToken();
      spotify.Play(context_uri);
      spotify.Shuffle();
      break;
    }
    default:
    {
      spotify.Shuffle();
      break;
    }
  }
}

bool readNFCTagData(byte *dataBuffer)
{
  MFRC522::StatusCode status;
  byte byteCount;
  byte buffer[18];
  byte x = 0;

  int totalBytesRead = 0;

  // reset the dataBuffer
  for (byte i = 0; i < BUFFERSiZE; i++)
  {
    dataBuffer[i] = 0;
  }

  for (byte page = 0; page < BUFFERSiZE / 4; page += 4)
  {
    // Read pages
    byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(page, buffer, &byteCount);
    if (status == mfrc522.STATUS_OK)
    {
      totalBytesRead += byteCount - 2;

      for (byte i = 0; i < byteCount - 2; i++)
      {
        dataBuffer[x++] = buffer[i]; // add data output buffer
      }
    }
    else
    {
      break;
    }
  }
}

/*
  Parse the Spotify link from the NFC tag data
  The first 28 bytes from the tag is a header info for the tag
  Spotify link starts at position 29


  Parse a link
  open.spotify.com/album/3JfSxDfmwS5OeHPwLSkrfr
  open.spotify.com/playlist/69pYSDt6QWuBMtIWSZ8uQb
  open.spotify.com/artist/53XhwfbYqKCa1cC15pYq2q


  Return a uri
  spotify:album:3JfSxDfmwS5OeHPwLSkrfr
  spotify:playlist:69pYSDt6QWuBMtIWSZ8uQb
  spotify:artist:53XhwfbYqKCa1cC15pYq2q

*/

String parseNFCTagData(byte *dataBuffer)
{
  // first 28 bytes is header info
  // data ends with 0xFE
  String retVal = "spotify:";
  for (int i = 28 + 17; i < BUFFERSiZE; i++)
  {
    if (dataBuffer[i] == 0xFE || dataBuffer[i] == 0x00)
    {
      break;
    }
    if (dataBuffer[i] == '/')
    {
      retVal += ':';
    }
    else
    {
      retVal += (char)dataBuffer[i];
    }
  }
  return retVal;
}

void hexDump(byte *dataBuffer)
{
  Serial.print(F("hexDump: "));
  Serial.println();
  for (byte index = 0; index < BUFFERSiZE; index++)
  {
    if (index % 4 == 0)
    {
      Serial.println();
    }
    Serial.print(dataBuffer[index] < 0x10 ? F(" 0") : F(" "));
    Serial.print(dataBuffer[index], HEX);
  }
}

void connectWifi()
{
  WiFi.begin(ssid, pass);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}