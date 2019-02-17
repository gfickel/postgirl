import json

import flask
from flask import Flask, request

app = Flask(__name__)


@app.route("/test_route", methods=['POST'])
def process_frapi_event():
    print("files", request.files)
    print("form", request.form)
    print("json", request.get_json(silent=True))

    curr_json = request.get_json(silent=True)
    if curr_json is not None:
        return flask.jsonify(curr_json)
    else:
        return '',200

@app.route("/version")
def version():
    return "1.0"

app.run(debug=True, host='0.0.0.0', port=5000)
print("Running app on port {}".format(5000))
