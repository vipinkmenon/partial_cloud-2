#!/usr/bin/python
# -*- coding: utf-8 -*-


import wx
import os
import shutil
from wx import html

class PrestoFrame(wx.Frame):
    def __init__(self,parent):
        self.title = "PRime"
        wx.Frame.__init__(self,parent,-1,self.title,wx.DefaultPosition,size=(560,630),style= wx.SYSTEM_MENU | wx.CAPTION | wx.CLOSE_BOX | wx.MINIMIZE_BOX)
        self.tool_pane = wx.Panel(self,-1,(0,45),(40,512),style= wx.TAB_TRAVERSAL|wx.NO_BORDER)
        self.image_pane = wx.Panel(self,-1,(40,45),(512,512),style= wx.TAB_TRAVERSAL|wx.NO_BORDER)
        self.tool_pane.SetBackgroundColour("gray")
        self.image_pane.SetBackgroundColour("white")
        self.initStatusBar()
        self.createMenuBar()
        self.createToolBar()
        self.Bind(wx.EVT_CLOSE,self.OnCloseWindow)
        text = wx.StaticText(self.tool_pane,-1,"FILT",(0,20),(50,20))
        self.createToolBox()
        self.State = 0
        self.in_filename = None
        self.filt_filename = 'filt_image.bmp'
        self.Show(True)      

    def initStatusBar(self):
        self.statusbar = self.CreateStatusBar()

    def menuData(self):
        data = [("&File",(
                  ("&Open",wx.ID_OPEN,"Open Image",self.OnOpen),
                  ("&Save",wx.ID_SAVE,"Save Image",self.OnSave),
                  ("&Save&As",wx.ID_SAVE,"Save Image As",self.OnSaveAs),
                  ("","","",""),
                  ("&Quit",wx.ID_EXIT,"Quit",self.OnCloseWindow))),
                ("&Edit",(
                  ("&Laplace",-1,"Laplace Filter",self.OnOpen), 
                  ("&Gaussian",-1,"Gaussian Filter",self.OnOpen),
                  ("&Sobel",-1,"Sobel Filter",self.OnOpen),
                  ("&Inverter",-1,"Inverter",self.OnOpen),
                  ("&Thresholder",-1,"Thresholder",self.OnOpen)
                )),
                ("&About",(
                  ("&About",-1,"About",self.OnAbout),
               ))]
        return data

    def createMenuBar(self):
        menuBar = wx.MenuBar()
        for eachMenuData in self.menuData():
            menuLabel = eachMenuData[0]
            menuItems = eachMenuData[1]
            menuBar.Append(self.createMenu(menuItems), menuLabel)
        self.SetMenuBar(menuBar)

    def createMenu(self,menuData):
        menu = wx.Menu()
        for eachItem in menuData:
            if len(eachItem) == 2:
                label = eachItem[0]
                subMenu = self.createMenu(eachItem[1])
                menu.AppendMenu(wx.NewId(), label, subMenu)
            else:
                self.createMenuItem(menu, *eachItem)
        return menu

    def createMenuItem(self,menu,label,item_id,status,handler,kind=wx.ITEM_NORMAL):
        if not label:
            menu.AppendSeparator()
            return
        menuItem = menu.Append(item_id,label,status,kind)
        self.Bind(wx.EVT_MENU,handler,menuItem)

    def OnCloseWindow(self,event):
        self.Destroy()

    def OnOpen(self,event):
        dlg = wx.FileDialog(self, "Open Image file...", os.getcwd(),style=wx.OPEN,wildcard="Image files (*.bmp) |*.bmp")
        if dlg.ShowModal() == wx.ID_OK:
            self.in_filename = dlg.GetPath()
        if self.in_filename:
            img = wx.Image(self.in_filename, wx.BITMAP_TYPE_ANY).ConvertToBitmap()
            imageCtrl = wx.StaticBitmap(self.image_pane, wx.ID_ANY,img,(0, 0),(img.GetWidth(), img.GetHeight()))
        dlg.Destroy()

    def OnSaveAs(self,event):
        dlg = wx.FileDialog(self,"Save project as...",os.getcwd(),style=wx.SAVE|wx.OVERWRITE_PROMPT)
        tmpfile = 'tmpfile'
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()
        if filename:
            try:
                with open(filename):
                    os.remove(filename)
            except:
                pass
            shutil.copyfile("receivedata.bmp",filename) #need to change here.
            self.in_filename = filename
            self.State = 1
            dlg.Destroy()


    def OnSave(self,event):
        if self.State == 1:
            filename = self.in_filename
            if filename:
                try:
                    with open(filename):
                        shutil.copyfile("receivedata.bmp",filename) #need to change here.
                except:
                    self.OnSaveAs(event)  
        else:
            self.OnSaveAs(event)

    def createToolBar(self):
        self.toolbar = wx.ToolBar(self,-1,(0,0),(512,50),wx.TB_HORIZONTAL)
        #self.toolbar.SetMargins( [4,4] ) 
        ftool = self.toolbar.AddLabelTool(wx.ID_ANY, 'Open', wx.Bitmap('icons/folder.png'),wx.NullBitmap, wx.ITEM_NORMAL, "","Open Image" ) 
        stool = self.toolbar.AddLabelTool(wx.ID_ANY, 'Save', wx.Bitmap('icons/save.png'),wx.NullBitmap, wx.ITEM_NORMAL, "", "Save Image" )
        itool = self.toolbar.AddLabelTool(wx.ID_ANY, 'Info', wx.Bitmap('icons/info.png'),wx.NullBitmap, wx.ITEM_NORMAL,"", "Info" )
        qtool = self.toolbar.AddLabelTool(wx.ID_ANY, 'Quit', wx.Bitmap('icons/exit.png'),wx.NullBitmap, wx.ITEM_NORMAL,"", "Quit" )
        self.toolbar.Realize()
        self.Bind(wx.EVT_TOOL, self.OnOpen, ftool)
        self.Bind(wx.EVT_TOOL, self.OnSave, stool)
        self.Bind(wx.EVT_TOOL, self.OnAbout, itool)
        self.Bind(wx.EVT_TOOL, self.OnCloseWindow, qtool)

    def OnAbout(self,event):
        dlg = PRestoAbout(self)
        dlg.ShowModal()
        dlg.Destroy()

    def OnFilter(self,event,filt_name,filt_type):
        try:
            with open(self.filt_filename):
                os.remove(self.filt_filename)
        except:
            pass
        bitstream = ' bitstreams/'+filt_name+'.bin '
        if filt_type == "s":
            sw = " swcode/stream_filt.c "
        else:
            sw = " swcode/conv_filt.c "
        command = 'scripts\client.exe 172.21.72.135 '+bitstream+sw+self.in_filename+' '+"receivedata.bmp"
        rtn = os.popen(command).readlines()
        #if len(rtn)==0:
        img = wx.Image("receivedata.bmp", wx.BITMAP_TYPE_ANY).ConvertToBitmap()
        imageCtrl = wx.StaticBitmap(self.image_pane, wx.ID_ANY,img,(0, 0),(img.GetWidth(), img.GetHeight())) 
        self.image_pane.Hide()
        self.image_pane.Show()
        #else:
        #    print 'Something went wrong'
        
    def createToolBox(self):     
        tfilt = wx.BitmapButton(self.tool_pane, 1, wx.Bitmap('icons/thresholder.png'),(0,50))
        ifilt = wx.BitmapButton(self.tool_pane, 2, wx.Bitmap('icons/inverter.jpeg'),(0,100))
        rfilt = wx.BitmapButton(self.tool_pane, 3, wx.Bitmap('icons/slicer.jpeg'),(0,150)) 
        lfilt = wx.BitmapButton(self.tool_pane, 4, wx.Bitmap('icons/laplace.gif'),(0,200)) 
        gfilt = wx.BitmapButton(self.tool_pane, 5, wx.Bitmap('icons/gaussian.png'),(0,260))
        sfilt = wx.BitmapButton(self.tool_pane, 6, wx.Bitmap('icons/sobel.png'),(0,310))
        bfilt = wx.BitmapButton(self.tool_pane, 7, wx.Bitmap('icons/box.jpeg'),(0,360))
        efilt = wx.BitmapButton(self.tool_pane, 8, wx.Bitmap('icons/emboss.jpeg'),(0,410))
      
        self.Bind(wx.EVT_BUTTON,lambda event: self.OnFilter(event,"thresholder","s"),tfilt)
        self.Bind(wx.EVT_BUTTON,lambda event: self.OnFilter(event,"inverter","s"),ifilt)
        self.Bind(wx.EVT_BUTTON,lambda event: self.OnFilter(event,"slicer","s"),rfilt)
        self.Bind(wx.EVT_BUTTON,lambda event: self.OnFilter(event,"laplace","c"),lfilt)
        self.Bind(wx.EVT_BUTTON,lambda event: self.OnFilter(event,"gaussian","c"),gfilt)
        self.Bind(wx.EVT_BUTTON,lambda event: self.OnFilter(event,"sobel","c"),sfilt)
        self.Bind(wx.EVT_BUTTON,lambda event: self.OnFilter(event,"box","c"),bfilt)
        self.Bind(wx.EVT_BUTTON,lambda event: self.OnFilter(event,"emboss","c"),efilt)
        
        
class PRestoAbout(wx.Dialog):
    text = '''
<html>
<body bgcolor = "#ACAA60">
<center><table bgcolor = "#455481" width="100%"cellspacing="0"
cellpadding="0" border-"1">
<tr>
<td align="center"><h1>PRime</h1></td>
</tr>
</table>
</center>
<p><b>PR based Image Manipulation Environment:</b> A demonstration platform for image processing using FPGA partial reconfiguration </b></p>
<p>Copyright &copy 2012-2013 Vipin K & Suhaib Fahmy, Nanyang Technological University, Singapore</p>
</body>
</html>
'''
    def __init__(self,parent):
        
        wx.Dialog.__init__(self,parent,-1,'About PResto',size=(400,300))
        html = wx.html.HtmlWindow(self)
        html.SetPage(self.text)
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(html,1,wx.EXPAND|wx.ALL,5)
        self.SetSizer(sizer)
        self.Layout()

def main():
  ex = wx.App()
  PrestoFrame(None)
  ex.MainLoop()

if __name__ == '__main__':
  main()
