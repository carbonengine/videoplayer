# Developing tips for working with videoplayer
This document will explain how to quickly iterate while making changes to the video player.

## Project location
All c++ code for the video player sits in ```carbon\src\Videoplayer```. You should use the ```carbon\src\carbon_v141.sln``` solution when working on it.

## Testing the video player
There are currently three ways to test changes you make to the videoplayer. 

### Test Method 1: player.py command line interface
This is the fastest method to iterate on video player changes because it does not require you restarting the client all the time. player.py in this directory is a command line interface that allows you pick a video and watch it play. 

#### Prerequisites
In order to be able to use player.py, you will need to do a few things.
1. Create a python 2 virtual environment.
2. Activate that virtual environment and pip install ```carbon\tools\launchUtil\requirements.txt```.

#### Usage
In a cmd window with the previously mentioned virtual environment activated, simply do
```python player.py -h```

and you should see the options available to you. The only required argument is ```--audio``` which tells it which audio sink to use.

#### Important things to keep in mind
player.py uses .pyd files. This means when you're working on the Videoplayer **you have to build Release_27** to see any of your changes.


#### Down sides
The biggest down side to using player.py is that the surrounding functionality is not what players will see in client and lacks volume control. 

### Test Method 2: Use the video player in the insider tool bar.
You can test the video player by starting a client and then navigating to ```Tools -> Video Player``` in the insider toolbar. 

#### Down sides
The down side to using the in game video player is that you will need to stop and restart the client every time you make a change.

This video player is also not the one that players will be seeing when looking at agency tutorial videos.

### Test Method 3: Use the video player that will be used for agency tutorial videos.
This method of testing will give you the most insight into the functionalities that an actual player will get when watching videos in the client. Open a client and navigate to ```Tools -> Python Console``` and then execute the following:

```uicore.cmd.OpenVideoPlayerWindow(videoPath="<res-path-to-video>")```

Where you would replace ```<res-path-to-video>``` with a path to a video using the ```res:/``` prefix. If you need a default video to use you can use ```res:/video/intro/intro.webm```.

#### Down sides
If you make a change you will need to kill and restart the client.