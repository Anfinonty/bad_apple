
void DrawPlayingMusic(HDC hdc,int x,int y,int c, int c4)
{
  if (!stop_playing_song) {

    if (song_num>0) {
      wchar_t txt[32+256];
      if (time_song_end==-1 && current_song_time==-1) {
        if (showoff%30<5 || showoff%30>25) {
          swprintf(txt,32+256,L"%c%d/%d%c: %s  [.  ]",171,song_rand_num+1,song_num,187,song_names[song_rand_num]);
        } else if ((showoff%30>4 && showoff%30<10) || showoff%30>19){
          swprintf(txt,32+256,L"%c%d/%d%c: %s  [.. ]",171,song_rand_num+1,song_num,187,song_names[song_rand_num]);
        } else {
          swprintf(txt,32+256,L"%c%d/%d%c: %s  [...]",171,song_rand_num+1,song_num,187,song_names[song_rand_num]);
        }

      } else {
        int songtimediff;
        if (current_song_time!=-1)
          songtimediff=(int)((time_song_end-current_song_time)/1000);
        else
          songtimediff=(int)((songAudio.duration)/1000);
        int min=songtimediff/60;
        int seconds=songtimediff%60;
        swprintf(txt,32+256,L"%c%d/%d%c: %s  [%d:%02d]",171,song_rand_num+1,song_num,187,song_names[song_rand_num],min,seconds);
      }
      //%c 187
      //wchar_t txt[32+256];
      //swprintf(txt,32+256,L"%c%d/%d%c: %s  []",171,song_rand_num+1,song_num,187,song_names[song_rand_num]);
      GrPrintW(hdc,x,y,txt,"",c,16,FALSE,TRUE);
      GrPrintW(hdc,x+1,y+1,txt,"",c4,16,FALSE,TRUE);

      

      char txt2[72];
      //char txt2_1[2046];
      //char txt2_2[256];
      /*for (int j=0;j<128;j++) {
        sprintf(txt2_1,"%s %d:%c ",txt2_1,'z'+j,'z'+j);
      }*/ //max 256
      //note %c 134 is a cross

      switch (song_mode) {
        case 0:
          sprintf(txt2,"[9%cSHIFT%c0]: %c] [M: /?] [/X :[N%cSHIFT%cM]: /%c]",171,187,177,171,187,187);
          break;
        case 1:
          sprintf(txt2,"[9%cSHIFT%c0]: %c] [M: /%c] [/? :[N%cSHIFT%cM]: /%c]",171,187,177,187,171,187,171);
          break;
        case 2:
          sprintf(txt2,"[9%cSHIFT%c0]: %c] [M: /%c] [/%c :[N%cSHIFT%cM]: /X]",171,187,177,171,187,171,187);
          break;
      }
      GrPrint(hdc,x,y+16,txt2,c);   
      GrPrint(hdc,x+1,y+1+16,txt2,c4);
    }
  } else {
    char txt2[72];
    sprintf(txt2,"[9%cSHIFT%c0]: %c] [M: /X] [/%c :[N%cSHIFT%cM]: /?]",171,187,177,171,171,187);
    GrPrint(hdc,x,y+16,txt2,c);   
    GrPrint(hdc,x+1,y+1+16,txt2,c4);
  }
}


void DrawMainMenu(HDC hdc)
{
  int help_y=GR_HEIGHT-128;
  DrawPlayingMusic(hdc,16+4,help_y+48,BLACK,WHITE);
}




