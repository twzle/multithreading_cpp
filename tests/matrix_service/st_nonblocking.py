#!/usr/bin/env python3

# TODO: Use testing framework

import os.path as path
import sys
import signal
import socket
import subprocess

from threading import Timer
from time import sleep


ROOT_DIR = path.dirname(__file__) + '/../../'
sys.path.append(ROOT_DIR + '/projects/protogen/py/')
import matrix_pb2
import matrix_service_pb2


BIN_FILE = ROOT_DIR + '/bin/matrix_service'
MODE = 'st_nonblocking'
ADDR = '127.0.0.1'
PORT = '23192' # FIXME: Generate
TIMEOUT = 2
THREADS = str(4)
ARGS = [BIN_FILE, '--server_type', MODE, '-a', ADDR, '-p', PORT, '-t', THREADS]

class TestServer:
    def kill(self):
        print('>>> KILLING SERVER: time is over')
        self.server.kill()

    def __init__(self, test_id, keepalive=False):
        print('Started test "' + test_id + '" ...')
        self.test_id = test_id

        self.server = subprocess.Popen(ARGS if not keepalive else ARGS + ['-k'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        sleep(0.1)

        self.timer = Timer(TIMEOUT, self.kill)

    def __enter__(self):
        self.timer.start()
        return self

    def finalize(self):
        try:
            self.server.send_signal(signal.SIGINT)
            self.stdout, self.stderr = self.server.communicate()
        finally:
            self.timer.cancel()

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.finalize()
        failed = self.server.returncode != 0 or exc_val is not None

        print('Test with id = "' + self.test_id + '" finished')
        if failed:
            print('>>> FAIL!!! Server returned != 0: (' + str(self.server.returncode) + ') <or> Exception, testname = ' + self.test_id)

        print('=== stdout ===')
        print(self.stdout)
        print('=== stderr ===')
        print(self.stderr)
        print('=== END: {} ==='.format('OK' if exc_val is None and not failed else 'FAILED') + '\n\n')

class Connection:
    def __init__(self):
        self.socket = None

    def send(self, msg, need_sleep=True):
        self.socket.sendall(msg)
        if need_sleep:
            sleep(0.05) # To make effect

    def send_request(self, serialized_proto):
        self.send(len(msg).to_bytes(4, 'little'), False) # TODO: Change to big endian
        self.send(serialized_proto)

    def try_recv(self):
        try:
            return self.socket.recv(1024)
        except BlockingIOError as err:
            if err.errno == 11: # EAGAIN
                return None
            raise err

    def __enter__(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((ADDR, int(PORT)))
        self.socket.setblocking(False)
        return self

    def __exit__(self, *args):
        if self.socket is not None:
            try:
                self.socket.shutdown(socket.SHUT_RDWR)
            except OSError as err:
                assert err.errno == 107 # Transport endpoint is not connected
            self.socket.close()


def make_matrix(m, val):
    m.rows = 1
    m.columns = 1
    m.content.append(val)

def make_mul_request(val1, val2):
    req_payload = matrix_service_pb2.MatrixOpRequest()
    req_payload.op = matrix_service_pb2.MatrixOpRequest.Operator.MUL
    make_matrix(req_payload.args.add(), val1)
    make_matrix(req_payload.args.add(), val2)

    req = matrix_service_pb2.ProcedureData()
    req.proc_id = matrix_service_pb2.ProcedureData.ProcedureId.MATRIX_OP
    req.payload = req_payload.SerializeToString()
    return req.SerializeToString()

def check_response(val, msg):
    resp = matrix_service_pb2.ProcedureData()
    resp.ParseFromString(msg)
    assert resp.proc_id == matrix_service_pb2.ProcedureData.ProcedureId.MATRIX_OP

    resp_payload_proto = matrix_service_pb2.MatrixOpResponse()
    resp_payload_proto.ParseFromString(resp.payload)
    assert resp_payload_proto.result.rows == 1
    assert resp_payload_proto.result.columns == 1
    assert len(resp_payload_proto.result.content) == 1
    assert resp_payload_proto.result.content[0] == val


# 1. Запуск - остановка
with TestServer("simple stop") as s:
    pass

# 2. Запуск - недописанное сообщение - остановка
with TestServer("cropped message") as s, Connection() as conn:
    conn.send(b'1') # Реально ждет 4 байта => сообщение не готово
    assert conn.try_recv() is None # TODO: Use testng framework

# 3. Запуск - нормальное сообщение - остановка
with TestServer("normal messages") as s, Connection() as conn:
    msg = make_mul_request(1, 2)
    conn.send_request(msg)
    check_response(2., conn.try_recv()[4:]) # Первые 4 байта - размер

    try:
        conn.socket.send(b'1')
        conn.socket.send(b'1')
        raise AssertionError('Without keepalive socket should be closed')
    except BrokenPipeError:
        pass # Ok

# 4. keepalive - 2 раза по 2 сообщения
with TestServer("keepalive", True) as s:
    for _ in range(2):
        with Connection() as conn:
            msg = make_mul_request(1, 2)
            for _ in range(2):
                conn.send_request(msg)
                check_response(2., conn.try_recv()[4:])
