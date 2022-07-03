from PyActor import IActor, Factory, gFactory


class TestActor(IActor):
    def __init__(self):
        super().__init__()

    def Launch(self):
        print("TestActor::Launch")


class AActor(IActor):
    def __init__(self):
        super().__init__()

    def Launch(self):
        print("测试用Actor")


class AAActor(IActor):
    def __init__(self):
        super().__init__()

    def Launch(self):
        print("测试用Actor(AAActor)")


def initialize():
    obj: Factory = gFactory()
    obj.Register("Test", TestActor)
    obj.Register("AActor", AActor)
    obj.Register("AAActor", AAActor)


def test():
    obj: Factory = gFactory()
    print(obj.Codes())

    actor: IActor = obj.Make("Test")
    if actor:
        actor.Launch()
    else:
        print("Invalid S")
    invalid: IActor = obj.Make("AActor")
    if invalid:
        invalid.Launch()
    else:
        print("Yes!")
    print("Invalid")

if __name__ == "__main__":
    initialize()
    test()
