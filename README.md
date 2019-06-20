# PageScrape
Code for a windows based tool called PageScrape (pscrape.exe) that was doing the rounds in the early 2000's

The tool was used for 'scraping' interesting bits off web pages using regular expressions.  Of course if you were using a Unix-like system you could just use wget | grep, but on windows you were not so lucky!

SOme text From the tool's (now defunct) website now follows:

## News

A PageScrape BLOG has been created, it can be found here

PageScrape Version 1.1 has finally been released, its new features are:

Support for Web Proxies.  Previous versions of PageScrape did not work through Web Proxies, version 1.1 does  automatically once the windows internet settings have been configured to use a proxy.
 
Support for multiple searches & matches within in a single web page (-m option).  Previous versions of PageScrape only returned one item of information per scrape - the first match found on a web page.  This version can be instructed to return all matches found,  this is handy for scraping lists of things from web pages.
 
Support for user specified formatted output (-f option).  PageScrape can be instructed to combine the results of its matches into an output string.  The format of this output string is dictated by the a user supplied formatting string.
 
Support for basic HTTP authentication (-a option).

## Description

PageScrape is a command line utility, which can be used to Screen Scrape specified data from a given target Web Page.  As an example consider retrieving the current stock price for your company from your favorite stock quote web page, or Screen Scraping the current temperature in Deli from one of the many weather web pages.  PageScrape can be easily invoked from other applications and scripts to further automate data gathering.

In order to let PageScrape to know how to scrape the required data the user provides the following parameters - target URL (along with optional HTTP GET request parameters) and a Regular Expression.  A Regular Expression is just a powerful and fairly standard way to express a set of textual search criteria, the provided Regular Expression is used by PageScrape to search the resulting HTML stream for the required data.

PageScrape connects to the Web server and submits a GET request, it waits to receive the resulting Web page (HTML text stream), and as it arrives PageScrape it searches using the provided Regular Expression, if a match occurs the matched data is output and the page download is stopped as the Web Screen Scrape is complete.


## Requirements:

Microsoft Internet Explorer 4 or greater.

## Installation

PageScrape is downloaded as a ZIP archive and extracts to a single executable pscrape.exe, there are no other installation steps required.   To make it easy to call PageScrape from the command-line it can be added to a directory which is on the system path.  PageScrape was designed to have as few runtime dependencies as possible, but we can't test it on all possible platform configurations, so if you do have any problems running PageScrape please drop us an e-mail.

## Using PageScrape

PageScrape is executed by running pscrape.exe, it has the following parameters:

-u<URL> [Required] - Specifies the target of the GET request, can include optional GET parameters after (after a '?') it will be the same as that displayed by your browser if you were to visit the target Web page.  If no protocol is specified 'http://' is assumed.  If the protocol is specified as 'file://' PageScrape will match the provided Regular Expression against the specified file, in this case any GET parameters are ignored.

-e<RegExp> [Required] - Specifies the Regular Expression to be used to search the resulting Web page in order to clip the required data.  By default Regular Expression searches are not greedy, to perform a greedy search use the -g option.

-b<BufIndex> [Optional] - The index of Regular Expression buffer which will be output by PageScrape on a successful match (if not specified this defaults to 1).

-o<OutputFie> [Optional] - If a match is found the resulting text is written to the specified OutputFile rather than to Stdout.

-l<LogFile> [Optional] - Log HTML stream, will write all of the downloaded HTML to LogFile

-f<FormatString> [Optional] - Will format the output according to the supplied format string.  Matches are inserted into the output string by referencing the required Regular Expression buffer number(s) in the format string as \$<bufNumber>.  Newline, Carriage Return and Tab can be specified as \n, \r and \t respectively.  For example, to scrape this page's title and place it in a very useful string:

pscrape -u"www.webscrape.com" -e"<title>([^<]*)</title>" -f"The Title is: \$1"

-a<UserName:Password> [Optional] - Instructs PageScrape to use basic HTTP authentication when retrieving a web page, the user name and password should be supplied for this option separated by a colon (:).

-g [Optional] - Greedy Search, specifies that the Regular Expression search is greedy, the default search method is not greedy.  When performing greedy searches, extra special care should be taken with using .* and other loose terms.  It may be easier to produce 'correct' searches if they are greedy but they may take longer to run than their non-greedy equivalents.

-i [Optional] - Specifies that the search should be carried out ignoring case, the default is not to ignore case.

-v [Optional] - Verbose mode, messages are output as PageScrape executes.

-p [Optional] - Print buffers, prints all of the Regular Expression buffers if a match occurs, this may be helpful for debugging search expressions.

-m [Optional] - This option instructs PageScrape to find and return all matches within the target web page.  It outputs each occurrence on a separate line of the output string.  If this option is not specified, PageScrape will just return the first match found.

To get a summary of PageScrape's options run pscrape.exe on the command line with no parameters, the following will be displayed:


pscrape -u<URL> -e<SearchExpr> [-b<BufIndex>] [-o<outputFile>] [-l<logFile>]
[-f<formatString>] [-a<Username:Password>] [-i] [-v] [-p] [-m]

-u URL, may include GET parameters, protocol is 'http://' by default,
if it is passed as 'file://' will match against the specified file.
-e Regular Expression to be matched with received data.
-b Index of RegEx buffer to be returned (defaults to 1).
-g Perform greedy search (default is non-greedy).
-i Search ignoring case.
-o Output to specified file rather than stdout.
-v Verbose mode, outputs progress messages.
-p Print Buffers, prints all of the matched RegEx Buffers.
-l Log received HTML to specified log file.
-f Format output as specified by a format string.
-a Specify Username and Password for HTTP Authentication,
Username and Password must be separated by : (a colon)
-m Search for multiple matches, the result of each match is output on a separate line.

PageScrape v1.1 (c) 2005
