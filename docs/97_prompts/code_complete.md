经过综合评估，当前的代码补全逻辑不好。
之前是由工具生成json文件，然后ksightCli会去读取json文件，然后生成commands_data.py。因为parser原生无法生成json，因此之前的做法是，定义一个struct，去填充parser以及生成json文件，这会导致parser不直观。
目前上述方案，我已经放弃。并且去除了工具生成json文件的代码。
新逻辑为：
1. 我会在每个工具的config文件夹下，放置一个bash-complete.sh的文件。它是工具开发者自己写的，使用complete命令来自动补全。这个bash-complete.sh，需要你帮我生成。根据当前工具的的命令行参数。
2. 同样的，在run.sh中，你需要复制每个config下的bash-complete.sh
3. 然后，你需要修改ksightCli的代码，让它可以去读取每个bash-complete.sh，然后生成commands_data.py
4. ksightCli然后继续处理自动补全逻辑。
5. 最后，你需要在run.sh中，添加自动补全到.bashrc文件中。例如命令：eval "$(_KSIGHTCLI_COMPLETE=source ksightCli)"