# Chat Protocol

## 1. 协议版本

当前版本：v1

本版本使用简单文本协议，主要用于学习 TCP 通信、客户端登录、群聊广播等基础流程。

当前协议格式：

```text
type|content
```

其中：

- `type` 表示消息类型。
- `content` 表示消息内容。
- `|` 是消息类型和内容之间的分隔符。

## 2. 客户端发送给服务端的消息

### 2.1 登录消息

格式：

```text
login|nickname
```

示例：

```text
login|Alice
```

含义：

客户端连接服务器后，首先发送登录消息。
服务端收到后，将该客户端 socket 对应的昵称保存起来。

服务端保存关系示例：

```text
client_fd -> Alice
```

### 2.2 聊天消息

格式：

```text
chat|message
```

示例：

```text
chat|hello world
```

含义：

客户端发送聊天内容。
服务端收到后，根据该客户端对应的昵称，将消息广播给其他在线客户端。

广播示例：

```text
Alice: hello world
```

## 3. 服务端广播给客户端的消息

当前版本中，服务端广播给客户端的是普通显示文本，不再带 `type|content` 格式。

### 3.1 用户加入聊天室

格式：

```text
nickname joined the chat
```

示例：

```text
Alice joined the chat
```

触发条件：

客户端发送合法的 `login|nickname` 消息后，服务端向其他在线客户端广播该消息。

### 3.2 聊天消息广播

格式：

```text
nickname: message
```

示例：

```text
Alice: hello world
```

触发条件：

客户端发送合法的 `chat|message` 消息后，服务端向其他在线客户端广播该消息。

### 3.3 用户离开聊天室

格式：

```text
nickname left the chat
```

示例：

```text
Alice left the chat
```

触发条件：

客户端正常断开连接，或服务端接收该客户端消息失败。

## 4. 服务端处理规则

服务端收到客户端消息后，根据消息前缀判断类型。

### 4.1 处理 login 消息

如果消息以：

```text
login|
```

开头，服务端认为这是登录消息。

处理流程：

1. 取出 `login|` 后面的内容作为昵称。
2. 如果昵称为空，忽略该消息。
3. 如果昵称合法，保存 `client_fd -> nickname`。
4. 向其他在线客户端广播：

```text
nickname joined the chat
```

### 4.2 处理 chat 消息

如果消息以：

```text
chat|
```

开头，服务端认为这是聊天消息。

处理流程：

1. 取出 `chat|` 后面的内容作为聊天内容。
2. 如果客户端尚未登录，忽略该消息。
3. 如果聊天内容为空，忽略该消息。
4. 如果消息合法，向其他在线客户端广播：

```text
nickname: message
```

### 4.3 处理未知消息

如果消息既不是 `login|` 开头，也不是 `chat|` 开头，服务端认为这是非法消息。

当前处理方式：

```text
忽略该消息
```

## 5. 客户端处理规则

客户端连接服务端后：

1. 输入昵称。
2. 发送登录消息：

```text
login|nickname
```

3. 进入聊天循环。
4. 用户输入聊天内容后，发送聊天消息：

```text
chat|message
```

5. 如果收到服务端广播消息，直接显示到终端。

## 6. 当前限制

当前 v1 协议是学习阶段的简化版本，存在以下限制：

- 没有消息长度字段，暂时没有解决 TCP 粘包和半包问题。
- `content` 中如果包含分隔符 `|`，当前版本没有做转义处理。
- 服务端广播消息目前是纯文本，不是统一的 `type|content` 格式。
- 暂时不支持私聊。
- 暂时不支持在线用户列表同步。
- 暂时不支持文件传输。
- 暂时没有统一错误消息格式。

## 7. 后续升级方向

后续协议可以升级为更清晰、更可靠的格式。

### 7.1 增加消息长度字段

计划格式：

```text
[length][payload]
```

其中：

- `length` 表示后面 payload 的字节长度。
- `payload` 表示真正的消息内容。

目的：

- 明确消息边界。
- 解决 TCP 粘包问题。
- 解决 TCP 半包问题。

### 7.2 使用结构化消息格式

后续可以使用 JSON 表示消息内容，例如：

```json
{
  "type": "chat",
  "from": "Alice",
  "content": "hello world"
}
```

优点：

- 字段含义更清楚。
- 扩展更方便。
- 更适合支持文件传输、在线用户列表、错误消息等功能。

### 7.3 未来可能支持的消息类型

```text
login       用户登录
chat        群聊消息
logout      用户退出
user_list   在线用户列表
file_meta   文件信息
file_chunk  文件分片
error       错误消息
```

## 8. 示例流程

### 8.1 Alice 登录

客户端发送：

```text
login|Alice
```

服务端广播：

```text
Alice joined the chat
```

### 8.2 Bob 登录

客户端发送：

```text
login|Bob
```

服务端广播：

```text
Bob joined the chat
```

### 8.3 Alice 发送聊天消息

Alice 客户端发送：

```text
chat|hello Bob
```

服务端广播：

```text
Alice: hello Bob
```

### 8.4 Bob 断开连接

服务端广播：

```text
Bob left the chat
```
