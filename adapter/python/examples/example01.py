import qbus

def main_get(qin):
  printf (qin)

def main_init(qbus):
  print("init")
  
  qbus.register ("GET", main_get)
  
  return 12
  
def main_done(obj):
  print(obj)

def main():

  dd = qbus.QBus()

  qbus.instance("EXAMPLE", main_init, main_done)

if __name__ == "__main__":
  main()
