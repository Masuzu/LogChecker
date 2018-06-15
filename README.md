# LogChecker

A little tool to compare two log files where each line is matched with regex patterns, configurable in an XML file (hard coded as 'config.xml', see main.cpp).

The extracted lines are binned into 'categories'. Then for each category, the matches retrived from the two files are compared against each other. A regex pattern extracts from each line capturing groups which form a compound key and other capturing groups which define the value for the match.

Motivation: I had some files to compare together in which even though the entries were mostly similar, it appeared that the order in which they occured were different. Given the number of entries to compare together and the fact that the value for each entry was identified by a group of ids (or a compound key), I find that it is a nifty tool for that purpose.

Usage: `./Logchecker <file1> <file2>`

The patterns to match into categories are read from the XML file `config.xml`.
Example:

The pattern `PAGC : {{uint}}{{skip}}- {{uint}}{{skip}}- {{double}}{{skip}}- {{uint}}` will match `PAGC : (uint) - (uint) - (double) - (uint)`

