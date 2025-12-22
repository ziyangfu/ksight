Connector是ksight用于连接前端UI交互的工具，后端通过Connector将数据推送给前端UI。并将前端的指令消息，下发给后端工具。
架构：
                          uds/tcp/http                        uds/tcp/http
[ksightCli netwatcher]    <-----------> Connector (server) <---------------> [ksight-ui client]
[ksightCli ......]        <---------------->|

开发语言：
Python / C++