import numpy
import json
import os
import pandas
from typing import Union

def save_npcbuf(arr: numpy.ndarray, filename: str):
    """Save npcbuf file from numpy array

    Args:
        arr (numpy.ndarray): [description]
        filename (str): [description]

    Returns:
        [type]: [description]
    """
    npjson = {
        "shape": arr.shape,
        "dtype": str(arr.dtype)
    }
    json_name = f"{filename}.json"
    npcbuf_name = f"{filename}.npcbuf"

    tmp_json = f"{json_name}.tmp"
    tmp_npcbuf = f"{npcbuf_name}.tmp"

    with open(tmp_json, "w") as fp:
        json.dump(npjson, fp)
    with open(tmp_npcbuf, "wb") as fp:
        fp.write(arr.tobytes(order='C'))

    os.rename(tmp_json, json_name)
    os.rename(tmp_npcbuf, npcbuf_name)

    return npcbuf_name, json_name

def load_npcbuf(filename: str, to_pandas: bool = False) -> Union[numpy.ndarray, pandas.DataFrame]:
    json_name = f"{filename}.json"
    npcbuf_name = f"{filename}.npcbuf"

    with open(npcbuf_name, "rb") as fp:
        buf = fp.read()

    with open(json_name, "r") as fp:
        npmeta = json.load(fp)

    npc_data = numpy.ndarray(shape=npmeta["shape"], dtype=npmeta["dtype"], order="C", buffer = buf)
    if to_pandas:
        dir_path = os.path.dirname(os.path.abspath(json_name))
        meta_path = os.path.join(dir_path, "meta.json")
        with open(meta_path, "r") as fp:
            metadata = json.load(fp)
        return pandas.DataFrame(npc_data,columns=metadata["colnames"])
    else:
        return npc_data

if __name__ == '__main__':
    # 20230323
    # npcdata[0]
    # array([10600000, 20230323, 93000000, 1], dtype=int32)
    # npcdata[242206]
    # array([11301439, 20230323, 150000000, 0], dtype=int32)
    npcdata=load_npcbuf("../../work/20230323",False)
    print(type(npcdata))
