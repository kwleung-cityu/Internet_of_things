# Part 3: Cloud Configuration with ThingSpeak

## What is the "Cloud"?

Before we begin, let's quickly define the "Cloud." In technology, the "cloud" refers to on-demand access to shared computing resources—like servers, storage, and software—delivered over the internet. For our project, ThingSpeak is a cloud service that will act as the "brain," allowing us to store, view, and react to our sensor data from anywhere in the world.

## Step 1: Create Your ThingSpeak Account and Channel

  First, we need to set up a destination for our data.

  1. **Create an Account:** Go to [ThingSpeak.com](https://thingspeak.mathworks.com/) and create a free account. You may need to verify your email address.

  2. **Create a New Channel:** Once logged in, click on **"Channels"** and then **"My Channels."** Click the **"New Channel"** button.

  3. **Configure the Channel:** Fill out the form with the following information:

     - **Name:** `Smart Flowerpot` (or any name you like)
     - **Description:** `Monitors soil moisture and controls a water pump.`
     - **Field 1:** Check the box to enable it and name it `Soil Moisture (%)`.
     - **Field 2:** Check the box to enable it and name it `Alarm Status`.
     - Scroll down and click **"Save Channel."**

     ![Screenshot of ThingSpeak channel settings]()

## Step 2: Find Your API Keys

API (Application Programming Interface) keys are like a secret username and password that allow your device to securely send data to your channel.

  1. After saving your channel, click on the **"API Keys"** tab.

  2. You will see two important keys:

     - **Write API Key:** This is the key your Arduino will use to *send* data to ThingSpeak.
     - **Read API Key:** This is used if you want to *read* data from ThingSpeak.

  3. **Copy the "Write API Key."** This is the most important key for now. You will need to paste this into your Arduino code later. **Keep it secret!**

     ![Screenshot of ThingSpeak API keys]()

## Step 3: Test Your Channel with a Web Browser (REST API)

Before writing any Arduino code, we can test that our channel is working by sending data directly from a web browser. This is a simple way to use the ThingSpeak REST API.

1. Copy your **Write API Key**.
2. Construct the following URL in a text editor, replacing `YOUR_API_KEY` with the key you just copied and choosing a test value (e.g., `55`).
3. Paste the complete URL into your web browser's address bar and press Enter.
4. If it works, you will see a number on the screen (which is the entry number).
5. Go back to your ThingSpeak channel and click on the **"Private View"** tab. You should see a new data point with the value `55` on your "Soil Moisture" chart! This confirms your channel is working correctly.

------

## Step 4: Set Up the Smartphone Alert

Now, let's make ThingSpeak send an alert when the plant is thirsty. We will use the "React" app.

1. In ThingSpeak, click on **"Apps"** and then select **"React."**
2. Click **"New React."**
3. Configure the React:
   - **Name:** `Low Moisture Alert`
   - **Condition Type:** `Numeric`
   - **Test Frequency:** `On data insertion`
   - **Condition:** `If channel [Your Channel ID] field [2] is equal to [1]`
   - **Action:** `ThingTweet` or `IFTTT` (You'll need to link your Twitter or IFTTT account. IFTTT is very powerful for sending notifications to different apps like Telegram, Pushbullet, or email).
   - **Options:** Set the text of the alert, for example: `Warning: Your plant is thirsty!`
4. Click **"Save React."**

Now, whenever your Arduino sends a `1` to Field 2, this React will trigger and send you the notification.
