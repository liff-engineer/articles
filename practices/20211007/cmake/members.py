from clang.cindex import Index, CursorKind
from typing import List, Dict, Any
import argparse
import os


def GetStructsAndEnumsInFile(file: str, classes: List[Dict[str, Any]]):
    idx = Index.create()
    tu = idx.parse(file)
    # 使用深度优先遍历当前节点
    for n in tu.cursor.walk_preorder():
        # 注意通过该方式屏蔽掉非本文件的抽象语法树节点
        if not n.location.file or n.location.file.name != file:
            continue
        # print(f"Cursor '{n.spelling}' of kind '{n.kind.name}'")
        # 如果是结构体定义则获取结构体类型名和成员变量名
        if n.kind == CursorKind.STRUCT_DECL:
            decls = []
            for i, m in enumerate(n.get_children()):
                decls.append({'decl': m.spelling})

            classes.append({
                'decl': n.type.spelling,
                'kind': 'struct',
                'members': decls
            })
        if n.kind == CursorKind.ENUM_DECL:
            decls = []
            for i in n.get_children():
                decls.append({'decl': i.spelling})

            classes.append({
                'decl': n.type.spelling,
                'kind': 'enum',
                'members': decls
            })


def GeneratorMembers(headers: List[str],
                     classes: List[Dict[str, Any]]) -> List[str]:
    lines = ['#pragma once']
    lines.append('')
    for header in headers:
        lines.append(f'#include "{header}"')

    lines.append('')

    for s in classes:
        name = s['decl']
        kind = s['kind']

        # 多行f string,注意对齐及换行
        lines.append(
            f'''template<>
struct abc::members<{name}>:std::true_type
{{
    template<typename Op>
    static void for_each(Op& op) {{''')

        for m in s['members']:
            mn = m['decl']
            if kind == 'enum':
                lines.append(f'\t\top({name}::{mn},"{mn}");')
            if kind == 'struct':
                lines.append(
                    f'\t\top(ABC_MAKE_MEMBER({name},{mn}));')

        lines.append(
            f'''    }}
}};
''')
    return lines


def GeneratorMembersFile(filename: str, headers: List[str],
                         classes: List[Dict[str, Any]]):
    with open(filename, "w", encoding='utf-8') as file:
        file.write('\ufeff')
        file.write('\n'.join(GeneratorMembers(headers, classes)))


def Run(files: List[str], result: str):
    headers = []
    for file in files:
        headers.append(os.path.basename(file))
    headers.append('members.hpp')

    classes = []
    for file in files:
        GetStructsAndEnumsInFile(file, classes)
    GeneratorMembersFile(result, headers, classes)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='为枚举、结构体生成成员遍历支持')
    parser.add_argument('files', nargs='+', type=str,
                        help='头文件列表')
    parser.add_argument(
        '-o', '--result', required=True, type=str, help='结果文件')

    args = parser.parse_args()
    Run(args.files, args.result)
