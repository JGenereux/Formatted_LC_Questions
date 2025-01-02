###
This is a very simple (in production) program that let's a user generate information
for a leetcode question.

It currently generates the following queries pertaining to the question and more can be easily added or they can be removed.
-title 
-content 
-difficulty 
-topicTags
-hints

The curl library is used in order to make get requests in c++.
It also uses nlohmann library for parsing json files

###
How to build this?
- Clone this repository
- In the terminal run the following commands in order! 
- mkdir build
- cd ./build
- cmake ..
- If your on windows run: msbuild ./GenLeetcodeQuestion.sln
- cd ./debug
- run main.exe and enter a question name to get a text file with the question formatted.

###
Enter questions with - replacing spaces and all characters undercase
ex. question name = "Two sum", search for two-sum instead.

###
Also the formatted file will still have extra newline characters (\n) like how the questions are on the site.
I left this optional as what I'm using for this is how I want it.