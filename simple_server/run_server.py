import json
import os
from pprint import pprint

import flask
from flask import Flask, request
from werkzeug.utils import secure_filename


os.makedirs("data", exist_ok=True)

app = Flask(__name__)

def format_response(request):
    files_list = []
    if request.files is not None:
        for f in request.files:
            files_list.append(f)
            file_obj = request.files[f]

            filename = secure_filename(file_obj.filename)
            file_obj.save(os.path.join('data', filename))


    response = {'json': request.get_json(silent=True),
                'headers': dict(request.headers),
                'args': request.args,
                'files': files_list,
                'form': request.form
                }
    pprint(response)
    return response


@app.route("/test_route", methods=['POST'])
def process_post():
    return flask.jsonify(format_response(request))


@app.route("/test_route", methods=['GET'])
def process_get():
    return flask.jsonify(format_response(request))


@app.route("/test_route", methods=['DELETE'])
def process_delete():
    return flask.jsonify(format_response(request))


@app.route("/test_route", methods=['PATCH'])
def process_patch():
    return flask.jsonify(format_response(request))


@app.route("/test_route", methods=['PUT'])
def process_put():
    return flask.jsonify(format_response(request))


@app.route("/version")
def version():
    return "1.0"

app.run(debug=True, host='0.0.0.0', port=5000)
print("Running app on port {}".format(5000))
