from EMJ.Production.jobSubmitterEMJ import jobSubmitterEMJ

def submitJobs():
    mySubmitter = jobSubmitterEMJ()
    mySubmitter.run()

if __name__=="__main__":
    submitJobs()