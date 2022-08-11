import requests, tarfile, io, os, tempfile, shutil



def fetch(path):
    return requests.get("https://nodejs.org/download/release/" + path).content



version = "17.4.0"
headers = fetch("v{0}/node-v{0}-headers.tar.gz".format(version))
lib =     fetch("v{0}/win-x64/node.lib".format(version))

try:
    os.remove("../node/")
except:
    pass
    
with tempfile.TemporaryDirectory() as tmp:
    obj = io.BytesIO(headers)
    tar = tarfile.open(fileobj=obj)
    tar.extractall(tmp)
    src = tmp + "/node-v{0}".format(version)
    
    for f in os.listdir(src):
        shutil.move(src + "/" + f, "../node/" + f)
    open("../node/node-win-x64.lib".format(version), "wb").write(lib)