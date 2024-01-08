import requests
import numpy
import json
import os
import pandas
from io import BytesIO
from typing import Union

def fetch_data(datetime: str, to_pandas: bool = False) -> Union[numpy.ndarray, pandas.DataFrame, str]:
    # need to edit in different environment
    main_server_url = "http://127.0.0.1:8888/" + datetime
    main_server_resp = requests.get(main_server_url)
    if main_server_resp.status_code < 200 or main_server_resp.status_code >= 300:
        return "request main server failed"
    ip = str(main_server_resp.text)
    if "failed" in ip :
        return "req main server: "+ ip

    # ip = "127.0.0.1:2333"
    ip_url = "http://" + ip + "/"

    json_url = ip_url + "json/" + datetime
    json_resp = requests.get(json_url)
    if json_resp.status_code < 200 or json_resp.status_code >= 300:
        return "http request json failed"
    npmeta = json_resp.json()

    npcbuf_url = ip_url + "npcbuf/" + datetime
    npcbuf_resp = requests.get(npcbuf_url)
    if npcbuf_resp.status_code < 200 or npcbuf_resp.status_code >= 300:
        return "http request npcbuf failed"
    npcbuf_str = npcbuf_resp.text
    npcbuf = BytesIO(npcbuf_str.encode('latin1'))
    buf = npcbuf.getvalue()

    npc_data = numpy.ndarray(shape=npmeta["shape"], dtype=npmeta["dtype"], order="C", buffer=buf)
    if to_pandas:
        meta_url = ip_url + "metajson"
        meta_resp = requests.get(meta_url)
        if meta_resp.status_code < 200 or meta_resp.status_code >= 300:
            return "http request meta json failed"
        metadata = meta_resp.json()
        return pandas.DataFrame(npc_data, columns=metadata["colnames"])
    else:
        return npc_data


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    data = fetch_data("20230323", False)

    if type(data) != str:
        print(type(data))
    else:
        print(data)
