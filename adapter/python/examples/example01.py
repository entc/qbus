import qbus

def main_get(qin):
  print (qin)

def main_put(qin):
  print (qin)

def main_init(qbus):
  
  qbus.register ("bts", main_get)

  demo1 = qbus.config ("demo1", 12)
  demo2 = qbus.config ("demo2", "hello world");
  demo3 = qbus.config ("demo3", True);
  
  print (demo1)
  print (demo2)
      
def main_done(qbus, obj):
  
  print("done")
  
  print(obj)

def main():

  qbus.instance("VELE", main_init, main_done)

if __name__ == "__main__":
  main()
