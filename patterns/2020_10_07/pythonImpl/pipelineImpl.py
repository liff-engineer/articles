from pynng import Pull0, Push0

url = 'ipc:///tmp/pipeline.ipc'
# 常规版本

node0 = Pull0()
node0.listen(url)

node1 = Push0()
node1.dial(url)
node1.send(b'Hello, World!')

print(node0.recv())

node1.close()
node0.close()

# 使用contentmanager的版本
with Pull0(listen=url) as node0, Push0(dial=url) as node1:
    node1.send(b'Hello, World!')
    print(node0.recv())
