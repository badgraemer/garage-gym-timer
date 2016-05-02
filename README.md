# garage-gym-timer
Arduino code for a DIY garage gym timer

Description of the 'menu' state machine system:

- Upon initial power on of the timer, a lower case 'm' will display in the left 2 digits of the display.
- At this point there are three valid inputs from the remote control (press '1', '2', or '3'), then subsequent options below:

1. Stopwatch function. Displays a number 1 in the right-most digit of the display
  * press 'back' to return to the main menu
  * press 'play' to start the stopwatch
    * press 'stop' to stop the stopwatch
    * press 'play' to start again
    * press 'back' to return to the stopwatch menu
2. Countdown Timer function
  * press 'up' arrow to increment minutes (up to 99)
  * press 'down' arrow to decrement minutes (down to 0)
  * press 'enter' to set the minutes to countdown. 'SET' will be displayed in the display
  * press 'play' to start the timer
    * press 'stop' to stop the timer
    * press 'play' to start again
    * press 'back' to return to the timer menu
3. Tabata timer function. This times 20 seconds of work and 10 seconds of rest and keeps track of the round of work/rest
  * press play to start the tabata timer
    * press 'stop' to stop the tabata timer
    * press 'play' to start again
    * press 'back' to return to the tabata timer menu
