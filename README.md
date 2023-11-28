# n-gram + discord
This is a CLI program that builds a n-gram language model for both users from their discord DM's.
## How to use it?
**C++20** and [Tyrrrz/DiscordChatExporter](https://github.com/Tyrrrz/DiscordChatExporter) are required.
1. Clone this repository `git clone git@github.com:kiletic/ngram-discord.git`
2. Enter the repository folder and export your discord DM's into a `data.txt` file using DiscordChatExporter CLI tool with the following command:  
`dotnet DiscordChatExporter.Cli.dll export --token "INSERT_YOUR_TOKEN_HERE" -c INSERT_CHANNEL_ID -f "PlainText" -o data.txt`  
**Note:** Make sure you enter your discord token into `INSERT_YOUR_TOKEN_HERE` and channel id into `INSERT_CHANNEL_ID`. [How to get the token and the channel_id?](https://github.com/Tyrrrz/DiscordChatExporter/blob/master/.docs/Token-and-IDs.md) 
3. Compile `main.cpp` with: `c++ -std=c++20 main.cpp -o main`
4. Run the program: `./main INSERT_USER1_NAME INSERT_USER2_NAME`  
  **Note**: inserted names should be actual **discord usernames** and not display names. It doesn't matter in which order you write them in.
