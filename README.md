### Nine Mile Assignment  
# To Build  
```
$ make
```  
# To Run  
In one terminal (omit -p for no event logging on the server side):  
```
$ ./event_replay -p StockData.csv  
```  
In another terminal:  
```  
$ ./imbalance_logger 1.0 out.txt
```  
Taking positional arguments percentage spread threshhold, and output file for QWAP computations.