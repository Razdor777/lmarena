import asyncio
import websockets
import json
import time
import hashlib

# OpCode из клиента
class OpCode:
    Work = 0
    CompleteWork = 1
    KeyIn = 2
    KeyOut = 3
    AuthFinish = 4
    IdentifyClient = 5
    IdentifyPlayer = 6
    ServerMessage = 7
    Error = 8
    Ping = 9
    Announcement = 10
    Join = 11
    Leave = 12
    Message = 13
    ListUsers = 14
    ConnectedUserList = 15
    IdentifySkinData = 16

class Client:
    def __init__(self, ws):
        self.ws = ws
        self.client_name = ""
        self.username = ""
        self.player_name = ""
        self.xuid = ""
        self.authenticated = False

# Все подключённые клиенты
clients: dict[websockets.WebSocketServerProtocol, Client] = {}

def make_op(opcode, data, success=True):
    """Создаёт ChatOp JSON"""
    return json.dumps({"o": opcode, "d": data, "s": success})

async def send_op(ws, opcode, data, success=True):
    """Отправляет операцию клиенту"""
    msg = make_op(opcode, data, success)
    try:
        await ws.send(msg)
    except:
        pass

async def broadcast(opcode, data, exclude=None):
    """Отправляет всем кроме exclude"""
    for ws, client in clients.items():
        if ws == exclude:
            continue
        if not client.authenticated:
            continue
        await send_op(ws, opcode, data)

async def broadcast_user_list():
    """Отправляет список пользователей всем"""
    user_list = {}
    i = 0
    for ws, client in clients.items():
        if not client.authenticated:
            continue
        user_list[str(i)] = {
            "0": client.client_name,
            "1": client.username,
            "2": client.player_name,
            "3": client.xuid
        }
        i += 1
    
    data = json.dumps(user_list)
    for ws, client in clients.items():
        if client.authenticated:
            await send_op(ws, OpCode.ConnectedUserList, data)

async def ping_loop():
    """Пингует всех клиентов каждые 5 секунд"""
    while True:
        await asyncio.sleep(5)
        for ws, client in list(clients.items()):
            if client.authenticated:
                try:
                    ts = str(int(time.time() * 1000))
                    await send_op(ws, OpCode.Ping, ts)
                except:
                    pass

async def handle_client(ws, path=None):
    """Обработка одного клиента"""
    client = Client(ws)
    clients[ws] = client
    
    addr = ws.remote_address
    print(f"[+] New connection from {addr}")
    
    try:
        # === ЭТАП 1: Аутентификация ===
        # Solstice клиент ожидает:
        # 1. KeyOut (но мы упростим — без шифрования)
        # 2. Work (proof of work)
        # 3. AuthFinish
        
        # Отправляем простой Work задачу
        # Клиент вызовет WorkingVM::SolveProofTask
        # Нам не важен результат — просто принимаем
        await send_op(ws, OpCode.Work, "1")  # простая задача
        
        # Ждём ответы от клиента
        async for raw_message in ws:
            try:
                data = json.loads(raw_message)
                opcode = data.get("o", -1)
                op_data = data.get("d", "")
                
                # === CompleteWork — клиент решил задачу ===
                if opcode == OpCode.CompleteWork and not client.authenticated:
                    # Принимаем любой ответ
                    await send_op(ws, OpCode.AuthFinish, "OK")
                    client.authenticated = True
                    print(f"[+] Client authenticated from {addr}")
                    continue
                
                # === KeyIn — клиент отправляет ключ ===
                if opcode == OpCode.KeyIn:
                    # Без шифрования — игнорируем
                    continue
                
                # === IdentifyClient ===
                if opcode == OpCode.IdentifyClient:
                    try:
                        info = json.loads(op_data)
                        client.client_name = info.get("0", "unknown")
                        print(f"[*] Client identified: {client.client_name}")
                    except:
                        pass
                    continue
                
                # === IdentifyPlayer ===
                if opcode == OpCode.IdentifyPlayer:
                    try:
                        info = json.loads(op_data)
                        old_name = client.username
                        client.username = info.get("0", "unknown")
                        client.player_name = info.get("1", "unknown")
                        client.xuid = info.get("2", "")
                        print(f"[*] Player: {client.player_name} ({client.username})")
                        
                        # Если это первая идентификация — отправляем Join
                        if not old_name:
                            join_msg = f"§a{client.username} ({client.player_name}) joined IRC"
                            await broadcast(OpCode.Join, join_msg)
                            await broadcast_user_list()
                    except:
                        pass
                    continue
                
                # === IdentifySkinData — скин, игнорируем ===
                if opcode == OpCode.IdentifySkinData:
                    continue
                
                # === Message — чат ===
                if opcode == OpCode.Message:
                    if not client.authenticated:
                        continue
                    
                    msg_text = f"§b{client.username}§f ({client.player_name}): {op_data}"
                    print(f"[MSG] {client.username}: {op_data}")
                    
                    # Отправляем всем включая отправителя
                    await broadcast(OpCode.Message, msg_text)
                    continue
                
                # === ListUsers ===
                if opcode == OpCode.ListUsers:
                    await broadcast_user_list()
                    continue
                
                # === Ping ===
                if opcode == OpCode.Ping:
                    # Клиент отвечает на наш пинг — всё ок
                    continue
                
                print(f"[?] Unknown opcode {opcode} from {client.username}: {op_data[:100]}")
                
            except json.JSONDecodeError:
                print(f"[!] Invalid JSON from {addr}")
            except Exception as e:
                print(f"[!] Error: {e}")
    
    except websockets.exceptions.ConnectionClosed:
        pass
    except Exception as e:
        print(f"[!] Connection error: {e}")
    finally:
        # Клиент отключился
        if client.authenticated and client.username:
            leave_msg = f"§c{client.username} ({client.player_name}) left IRC"
            del clients[ws]
            await broadcast(OpCode.Leave, leave_msg)
            await broadcast_user_list()
            print(f"[-] {client.username} disconnected")
        else:
            if ws in clients:
                del clients[ws]
            print(f"[-] {addr} disconnected")

async def main():
    port = 33651
    print(f"=== Solstice IRC Server ===")
    print(f"Starting on 0.0.0.0:{port}")
    print(f"Waiting for connections...")
    print()
    
    # Запускаем пинг
    asyncio.create_task(ping_loop())
    
    # Запускаем сервер
    async with websockets.serve(handle_client, "0.0.0.0", port):
        await asyncio.Future()  # run forever

if __name__ == "__main__":
    asyncio.run(main())
