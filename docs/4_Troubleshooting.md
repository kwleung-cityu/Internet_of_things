# Troubleshooting

**1. No message on Serial Monitor?**

	Possible causes: 

   * The baud rate of the Serial Monitor does not match with the configuration in the program

     e.g. in `setup() {}` you used `Serial.begin(9600)` but the Serial Monitor set to  a baud rate of 115200bps.
	
   * `Serial.begin()` is never called
   * `A broken USB cable`.

**2. No data uploading from my Arduino device to the Cloud**

	Possible causes: 

* API key is not correctly set
* The REST API call is not correct

**3. Measurement results not stable**

	Possible causes:
	
* Loose jumper cables
* Software bug


