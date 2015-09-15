# Muvr

[muvr](http://www.muvr.io/) is a demonstration of an application that uses _wearable devices_ (Pebble)—in combination with a mobile app—to submit physical (i.e. accelerometer, compass) and biological (i.e. heart rate) information to a CQRS/ES cluster to be analysed.

#### muvr-pebble
`muvr-pebble` is a small pebble watch fitness tracking application. It captures movement data, displays classification results and keeps track of your exercise plan.  

This part of the project is written in plain old C.

#### Other components of the system
- [muvr-server](https://github.com/muvr/muvr-server) CQRS/ES cluster 
- [muvr-ios](https://github.com/muvr/muvr-ios) iOS application showcasing mobile machine learning and data collection
- [muvr-preclassification](https://github.com/muvr/muvr-preclassification) mobile data processing and classification
- [muvr-analytics](https://github.com/muvr/muvr-analytics) machine learning model generation for movement analytics

## Getting started
Basic information to get started is below. Please also have a look at the other components of the system to get a better understanding how everything fits together.

### Installation
To be able to compile, build and ship apps to the pebble watch, you need to install the developer tools.
```
brew install pebble/pebble-sdk/pebble-sdk
```
More information is provided on the [pebble developer site](https://developer.getpebble.com/sdk/). You need to install the pebble app on your phone and activate the `Developer` mode. 


In general there are to steps to get your code running on a pebble:
  1. Compile and package the code using `pebble build`
  2. Install the app on the pebble using `pebble install --phone 192.168.0.42`

To successfully install the app on your pebble, you need to navigate to the developer pane in the pebble app of your phone. This pane will also show you the ip address you need to use in the second command.

### Issues

For any bugs or feature requests please:

1. Search the open and closed
   [issues list](https://github.com/muvr/muvr-pebble/issues) to see if we're
   already working on what you have uncovered.
2. Make sure the issue / feature gets filed in the relevant components (e.g. server, analytics, pebble)
3. File a new [issue](https://github.com/muvr/muvr-pebble/issues) or contribute a 
  [pull request](https://github.com/muvr/muvr-pebble/pulls) 

## License
Please have a look at the [LICENSE](https://github.com/muvr/muvr-pebble/blob/develop/LICENSE) file.
