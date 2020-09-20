import sys
import os
from clang.cindex import *
from jinja2 import Template
from typing import List, Dict, Any

# idx = Index.create()
# tu = idx.parse(sys.argv[1], ['-std=c++11'])

# for n in tu.cursor.walk_preorder():
#     print(f"Cursor '{n.spelling}' of kind '{n.kind.name}'")
#     if n.kind == CursorKind.ENUM_DECL:
#         print(f"enum declare:{n.spelling}")
#         for i in n.get_children():
#             print(f"{n.type.spelling}::{i.spelling}")
#     if n.kind == CursorKind.STRUCT_DECL:
#         print(f"struct declare:{n.spelling}")
#         for i, m in enumerate(n.get_children()):
#             print(f"{m.spelling}- {i}")

#     if n.kind == CursorKind.CLASS_DECL:
#         print(f"class declare:{n.spelling}")
#         for i, m in enumerate(n.get_children()):
#             if m.kind == CursorKind.CXX_METHOD:
#                 print(f"{m.spelling}")
#                 if m.is_pure_virtual_method():
#                     print(f"abstract method")
#                 if m.is_virtual_method():
#                     print(f"virtual method")


def extract_enum_and_struct(file: str, enums: List[Dict[str, Any]], structs: List[Dict[str, Any]]):
    idx = Index.create(excludeDecls=True)
    tu = idx.parse(file)
    for n in tu.cursor.walk_preorder():
        # 移除非本文件的定义
        if not n.location.file or n.location.file.name != file:
            continue
        print(f"Cursor '{n.spelling}' of kind '{n.kind.name}'")
        # 如果是枚举定义则获取枚举类型名和值列表
        if n.kind == CursorKind.ENUM_DECL:
            values = []
            for i in n.get_children():
                values.append(i.spelling)
            enums.append({
                'decl': n.spelling,
                'values': values
            })
        # 如果是结构体定义则获取结构体类型名和成员变量名
        if n.kind == CursorKind.STRUCT_DECL:
            members = []
            for i, m in enumerate(n.get_children()):
                members.append({'decl': m.spelling})

            structs.append({
                'decl': n.spelling,
                'members': members
            })


if __name__ == "__main__":
    headers = ["example.hpp"]
    # enums = [
    #     {
    #         'decl': "MyType",
    #         'values': ["A", "B", "C"]
    #     }
    # ]
    # structs = [
    #     {
    #         'decl': "Address",
    #         'members': [
    #             {
    #                 'decl': "zipcode"
    #             },
    #             {
    #                 'decl': "detail"
    #             }
    #         ]
    #     }
    # ]
    enums = []
    structs = []
    for header in headers:
        extract_enum_and_struct(header, enums, structs)

    with open('template/ostream.jinja', encoding='utf-8') as file:
        template = Template(file.read(), trim_blocks=True, lstrip_blocks=True)
        template.stream(headers=headers, enums=enums,
                        structs=structs).dump(f'example_ostream.hpp')
