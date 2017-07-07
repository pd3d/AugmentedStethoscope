"""


"""

from        scipy.io.wavfile        import  read

import      Tkinter
import      tkFileDialog
import      numpy                   as      np
import      matplotlib.pyplot       as      plt




# Browse WAV file
def browse_WAV():
    root = Tkinter.Tk()
    file = tkFileDialog.askopenfile(parent=root,mode='rb',title='Choose a file')
    #if file != None:
    #    data = file.read()
    #    file.close()
    #    #print "I got %d bytes from this file." % len(data)
    root.destroy()
    #return file, data
    return file


# Read WAV file
def read_WAV(wav_file):
    wav_struct = dict()                                                                     # data structure
    raw_data = read(wav_file)                                                               # reading the wav file
    wav_struct['raw'] = raw_data                                                            # store raw output from the read() function
    wav_struct['fs'] = raw_data[0]                                                          # sampling rate (Hz)
    wav_struct['amp'] = raw_data[1]                                                         # amplitude (signal) 
    wav_struct['n_samples'] = len(raw_data[1])                                              # number of samples in signal
    wav_struct['tf'] = len(raw_data[1])/float(raw_data[0])                                  # time length of recording
    wav_struct['dt'] = 1./raw_data[0]                                                       # time interval
    wav_struct['t'] = np.linspace(0,len(raw_data[1])/float(raw_data[0]),len(raw_data[1]))   # approximation of time array (using linespace)

    return wav_struct

# Plot WAV file
def plot_WAV(wav_struct):

    plt.plot(wav_struct['amp'])
    plt.show()

"""
References
1- https://docs.python.org/2/library/wave.html
"""
