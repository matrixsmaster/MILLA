ANNA_PATH = ../../anna

cublas = $$(USE_CUBLAS)
equals(cublas,1) {
    DEFINES += GGML_USE_CUBLAS
    LIBS += -L/usr/local/cuda/lib64 -L/opt/cuda/lib64 -lcuda -lcublas -lculibos -lcudart -lcublasLt
}