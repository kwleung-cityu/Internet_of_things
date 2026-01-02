# Part 3: Cloud Configuration with ThingSpeak

## What is the "Cloud"?

Before we begin, let's quickly define the "Cloud." In technology, the "cloud" refers to on-demand access to shared computing resources—like servers, storage, and software—delivered over the internet. For our project, ThingSpeak is a cloud service that will act as the "brain," allowing us to store, view, and react to our sensor data from anywhere in the world.



## Step 1: Create Your ThingSpeak Account and Channel

  First, we need to set up a destination for our data.

  1. **Create an Account:** Go to [ThingSpeak.com](https://thingspeak.mathworks.com/) and create a free account. You may need to verify your email address.

  2. **Create a New Channel:** Once logged in, click on **"Channels"** and then **"My Channels."** Click the **"New Channel"** button.

     <img src = "./images/creating_new_channels.png">

  3. **Configure the Channel:** Fill out the form with the following information:

     - **Name:** `Smart Flowerpot` (or any name you like)
     - **Description:** `Monitors soil moisture and controls a water pump.`
     - **Field 1:** Check the box to enable it and name it `Soil Moisture (%)`.
     - Scroll down and click **"Save Channel."**

     <img src = "./images/channel_setup.png">

## Step 2: Find Your API Keys

API (Application Programming Interface) keys are like a secret username and password that allow your device to securely send data to your channel.

  1. After saving your channel, click on the **"API Keys"** tab.

  2. You will see two important keys:

     - **Write API Key:** This is the key your Arduino will use to *send* data to ThingSpeak.
     - **Read API Key:** This is used if you want to *read* data from ThingSpeak.

  3. **Copy the "Write API Key."** This is the most important key for now. You will need to paste this into your Arduino code later. **Keep it secret!**

     <img src = "./images/API_keys.png">

## Step 3: Test Your Channel with a Web Browser (REST API)

Before writing any Arduino code, we can test that our channel is working by sending data directly from a web browser. This is a simple way to use the ThingSpeak **REST API** - **Representational State Transfer Application Programming Interface**. The **REST-API** is a set of rules for how computer systems can exchange resources (photos, a blog post, sensor readings, etc) to each other over the Internet.

1. Copy your **Write API Key**.

2. Construct the following URL in a text editor, replacing `YOUR_API_KEY` with the **Write API key** you just copied and choose a test value (e.g., `55`).

   ```http
   https://api.thingspeak.com/update?api_key=YOUR_API_KEY&field1=55
   ```

3. Paste the complete URL into your web browser's address bar and press Enter.

   <img src = "./images/url_to_write.png">

4. Go back to your ThingSpeak channel and click on the **"Private View"** tab. You should see a new data point on **Field1 (Soil Moisture %)** with the value **55** . This confirms your channel is working correctly.

   <img src = "./images/channel_write_OK.png">

------

## Step 4: Set Up the Smartphone Alert

Now, let's make ThingSpeak send an alert when the plant is thirsty. We need to use three services to make this work:

* **CallMeBot** - This is a versatile web service and API that allows users to send automated notifications, text messages, and even voice calls to popular messaging platforms and standard phone lines. In this demo we are going to configure WhatsApp Messages to send from CallMeBot when the plant is thirsty.

* **ThingHTTP** app of ThingSpeak - This is the **"Action."** Its job is to make an HTTP request to CallMeBot to make an instant notification to a registered phone number

* **React** app of ThingSpeak - This is the **"Trigger."** Its job is to constantly watch your channel's data - "WHEN `Soil Moisture (%)` of my channel becomes `less than 20`, call ThingHTTP app"

  #### **Step 4A: Configure CallMeBot**

  1. Add the phone number **+34 694 23 67 31** into your Phone Contacts. (Name it WhatsApp Bot, say )

  2. Send this message "**I allow CallMeBot to send me messages**" to the new Contact created using WhatsApp

  3. You will receive the message "**CallMeBot API Activated for [your_phone_number]**". Your apikey is: **[your_Apikey]**.

     **Note:** This is the CallMeBot apikey, **NOT** the ThingSpeak write apikey you copied earlier. You will get several API keys for this demo.
     
     <img src = "./images/callmebot_activated.png">
     
  4. You can now send a message using REST API calls to test it . Construct the following URL in a text editor and paste the complete URL `https://api.callmebot.com/whatsapp.php?phone=[your_phone_number]&text=This+is+a+test&apikey=[your_CallMeBot_apikey]` into your web browser's address bar and press Enter.

     **Example:**

     <img src = "./images/callmebot_rest_message.png">

     <img src = "./images/callmebot_this_is_a_test.png">

  #### **Step 4B: Configure the `ThingHTTP` Action**

  Now, we have **CallMeBot** ready. The next step is to teach ThingSpeak *how* to talk to the CallMeBot API which is the HTTP message you entered into the web browser but this time, we need to do it automatically.

  1. In ThingSpeak, go to the **"Apps"** menu and select **"ThingHTTP."**

  2. Click **"New ThingHTTP."**

     <img src = "./images/ThingHTTP_config_1.jpg" width=60%>

  3. Fill out the form with the following details:

     - **Name:** `Send WhatsApp Alert`

     - **URL:** This is the most important part. You need to construct the entire API request URL here, including your phone number, the message, and your API key. Replace the placeholders `[]` with your actual values.
       
       ```
       https://api.callmebot.com/whatsapp.php?phone=[Your_Phone_Number]&text=Warning:+Your+plant+is+thirsty!&apikey=[Your_CallMeBot_API_Key]
       ```
       **Important:** In this demo, we are going to send a text message - *Warning: Your plant is thirsty* to your smartphone by WhatsApp. Notice the `+` signs in the `text` parameter. In a URL, you cannot have spaces. The `+` character is used to represent a space.
       
     - **Method:** `GET`

     - **Content Type:** `application/x-www-form-urlencoded`

     - **HTTP Version:** `1.1`

     - **Host, Body & Parse String:** Leave those fields **blank**.

  4. Click **"Save ThingHTTP."**

  <img src = "./images/ThingHTTP_config_2.png">

  5. Test ThingHTTP by entering the following URL `https://api.thingspeak.com/apps/thinghttp/send_request?api_key=[Your_ThingHTTP_apikey]`  into your web browser's address bar and press Enter. 

     <img src = "./images/ThingHTTP_Url_Test.png">

     <img src = "./images/ThingHTTP_Url_Test_result.png">
  
  Receiving the correct WhatsApp alert confirms that your ThingHTTP-to-CallMeBot action chain is configured properly.

  #### **Step 4C: Configure the `React` Trigger**

  The final step is to create the trigger that automatically initiates the alert. The `React` app will monitor the `Soil Moisture (%)` field and execute the `ThingHTTP` action when the moisture level drops below a specific threshold, in our case 20%.

  1. Go to the **"Apps"** menu and select **"React."**

  2. Click **"New React."**

     <img src = "./images/react_config_1.png">
  
  3. Fill out the form with the following details:
     - **Name:** `Low Moisture Alert`
     - **Condition Type:** `Numeric`
     - **Test Frequency:** Select `On data insertion`. This ensures the condition is checked every time your device sends new data.
     - **Condition:** Configure it as follows:
       - `If channel [Your Channel ID] field [1] is less than [20]`
       - This tells the React to trigger when the value in **Field 1 (Soil Moisture %)** drops below **20**.
     - **Action:** Select `ThingHTTP` from the dropdown menu.
     - **ThingHTTP to execute:** A new dropdown will appear. Select the **`Send WhatsApp Alert`** action you created earlier.
     - **Options:** Select `Run action only the first time the condition is met`. This prevents you from receiving repeated alerts every time a new data point below the threshold is inserted. The alert will reset and can be triggered again once a value above the threshold is received.
     
  4. Click **"Save React."**

     <img src = "./images/react_config_2.png">
  
  #### **Step 4D: Test the Full Alert System**
  
  With all the cloud components configured, it's time for the final test. We will now manually trigger the `React` condition to ensure the entire alert chain works as expected. This process simulates what your Arduino will do automatically when it detects low moisture.
  
  1. **Construct the Test URL:** In a text editor, create the following URL. This will send a value of `19` to `field1`, which is below our threshold of `20` and should trigger the alert.
  
     `https://api.thingspeak.com/update?api_key=YOUR_WRITE_API_KEY&field1=19`
  
     Remember to replace `YOUR_WRITE_API_KEY` with your channel's **Write API Key**.
  
  2. **Trigger the Alert:** Paste the complete URL into your web browser's address bar and press Enter.
  
     <img src = "./images/react_thingHTTP_callmebot_test1.png">
  
  **Expected Result:**
  
  Your channel will be updated with the new moisture value you manually set with `field1=19`.
  
  <img src = "./images/react_thingHTTP_callmebot_test1.1.png">
  
  Within a minute or two, you should receive a WhatsApp message on your phone that says: "Warning: Your plant is thirsty!"
  
  <img src = "./images/react_thingHTTP_callmebot_test2.png">
  
  Receiving this message confirms that your entire cloud setup is working perfectly!

---

## Conclusion: Your Cloud Platform is Ready!

Congratulations! You have successfully configured a complete, end-to-end cloud monitoring and alert system.

- You created a **ThingSpeak channel** to store and visualize your sensor data.
- You learned how to use **API keys** to securely communicate with the service.
- You configured a **ThingHTTP** action to connect ThingSpeak to an external notification service (CallMeBot).
- You set up a **React** to automatically trigger that action based on your data.

Your cloud platform is now fully prepared. It is waiting to receive data from your physical device and will automatically send an alert to your phone when your plant needs water.

The final piece of the puzzle is to program your Arduino to read the moisture sensor and send the data to ThingSpeak.

**Next Step:** Proceed to [Part 3: Uploading Data to the Cloud](../docs/2_Software_Development.md#part-3-uploading-moisture-data-to-thingspeak) to begin writing the device code.
