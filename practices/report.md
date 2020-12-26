打开网页
```python
from selenium import webdriver
from bs4 import BeautifulSoup

def asTable(html):
    soup = BeautifulSoup(html, 'html.parser')
    results = []
    for row in soup.find_all('tr'):
        if row.has_attr('hidden'):
            continue
        aux = row.find_all('td')
        if not aux:
            aux = row.find_all('th')
        items = []
        for e in aux:
            e = e.find('span')
            items.append(str(e.string).replace(u"\u2002", ''))
        results.append(items)
    return list(map(list, zip(*results)))

browser = webdriver.Chrome()
browser.get("http://f10.eastmoney.com/f10_v2/FinanceAnalysis.aspx?code=sz300359")
```
切换到按年份
```python
e = browser.find_element_by_id("zcfzb_ul")
e = e.find_element_by_xpath("//li[@reportdatetype='1']")
e.click()
browser.implicitly_wait(10)
e = browser.find_element_by_id("zcfzb_ul")
e = e.find_element_by_xpath("//li[@reportdatetype='1']")
e.click()
browser.implicitly_wait(10)
```

获取报表
```python
results = []
target = browser.find_element_by_id("report_zcfzb")
result = target.get_attribute('innerHTML')
results.append(asTable(result))
while True:
    e = browser.find_element_by_id("zcfzb_next")
    if e and e.is_displayed() and e.is_enabled():
        e.click()
        browser.implicitly_wait(10)
        
        target = browser.find_element_by_id("report_zcfzb")
        result = target.get_attribute('innerHTML')
        results.append(asTable(result))
    else:
        break
```

转换成数据表
```python
lines = set()
table = []
for l1 in results:
    for row in l1:
        key = row[0]
        if key in lines:
            continue
        lines.add(row[0])

        items = []
        for item in row:
            if '亿' in item:
                items.append(float(str(item).replace('亿',''))*10000)
            elif '万' in item:
                items.append(float(str(item).replace('万','')))       
            else:
                items.append(item)

        table.append(items)

table
```

创建dataframe并保存
```python
import pandas as pd 

df = pd.DataFrame(table)
df
df.to_excel(r'table1.xlsx')
```
