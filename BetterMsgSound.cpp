//---------------------------------------------------------------------------
// Copyright (C) 2014 Krzysztof Grochocki
//
// This file is part of BetterMsgSound
//
// BetterMsgSound is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// BetterMsgSound is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Radio; see the file COPYING. If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street,
// Boston, MA 02110-1301, USA.
//---------------------------------------------------------------------------

#include <vcl.h>
#include <windows.h>
#pragma hdrstop
#pragma argsused
#include <PluginAPI.h>
#include <inifiles.hpp>

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
	return 1;
}
//---------------------------------------------------------------------------

//Struktury-glowne-----------------------------------------------------------
TPluginLink PluginLink;
TPluginInfo PluginInfo;
//Lista-JID-otwartych-zakladek-----------------------------------------------
TStringList *TabsList = new TStringList;
//Lista-JID-kontaktow-od-ktorych-otrzymano-wiadomosc-------------------------
TStringList *MsgsList = new TStringList;
//Tryb-auto-oddalenia--------------------------------------------------------
bool AutoAwayMode = false;
//Uchwyt-do-okna-rozmowy-----------------------------------------------------
HWND hFrmSend;
//FORWARD-AQQ-HOOKS----------------------------------------------------------
INT_PTR __stdcall OnActiveTab(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnAutoAwayOff(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnAutoAwayOn(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnCloseTab(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnFetchAllTabs(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnPlaySound(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnPrimaryTab(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnRecvMsg(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnWindowEvent(WPARAM wParam, LPARAM lParam);
//---------------------------------------------------------------------------

//Sprawdzanie czy dziwieki zostaly wlaczone
bool ChkSoundEnabled()
{
	TStrings* IniList = new TStringList();
	IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP,0,0));
	TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
	Settings->SetStrings(IniList);
	delete IniList;
	UnicodeString SoundOff = Settings->ReadString("Sound","SoundOff","0");
	delete Settings;
	return !StrToBool(SoundOff);
}
//---------------------------------------------------------------------------

//Pobieranie sciezki kompozycji
UnicodeString GetThemeDir()
{
	return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETTHEMEDIR,0,0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------

//Pobieranie sciezki domyslnej kompozycji
UnicodeString GetDefaultThemeDir()
{
	return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETAPPPATH,0,0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll) + "\\\\System\\\\Shared\\\\Themes\\\\Standard";
}
//---------------------------------------------------------------------------

//Pobieranie adresu do pliku dzwiekowego pierwszej nowej wiadomosci
UnicodeString GetFirstSoundPath()
{
	//Pobieranie sciezki kompozycji
	UnicodeString ThemeDir = GetThemeDir();
	//Plik MP3 z aktywnej kompozycji
	if(FileExists(ThemeDir+"\\\\\Sound\\\\First.mp3"))
		return ThemeDir+"\\\\\Sound\\\\First.mp3";
	//Plik WAV z aktywnej kompozycji
	else if(FileExists(ThemeDir+"\\\\\Sound\\\\First.mp3"))
		return ThemeDir+"\\\\\Sound\\\\First.mp3";
	//Pliki w domyslnej kompozycji
	else
	{
		//Pobieranie sciezki domyslnej kompozycji
		ThemeDir = GetDefaultThemeDir();
		//Plik MP3 z domyslnej kompozycji
		if(FileExists(ThemeDir+"\\\\\Sound\\\\First.mp3"))
			return ThemeDir+"\\\\\Sound\\\\First.mp3";
		//Plik WAV z domyslnej kompozycji
		else if(FileExists(ThemeDir+"\\\\\Sound\\\\First.mp3"))
			return ThemeDir+"\\\\\Sound\\\\First.mp3";
	}
	return "";
}
//---------------------------------------------------------------------------

//Pobieranie adresu do pliku dzwiekowego nowej wiadomosci
UnicodeString GetInSoundPath()
{
	//Pobieranie sciezki kompozycji
	UnicodeString ThemeDir = GetThemeDir();
	//Plik MP3 z aktywnej kompozycji
	if(FileExists(ThemeDir+"\\\\\Sound\\\\In.mp3"))
		return ThemeDir+"\\\\\Sound\\\\In.mp3";
	//Plik WAV z aktywnej kompozycji
	else if(FileExists(ThemeDir+"\\\\\Sound\\\\In.mp3"))
		return ThemeDir+"\\\\\Sound\\\\In.mp3";
	//Pliki w domyslnej kompozycji
	else
	{
		//Pobieranie sciezki domyslnej kompozycji
		ThemeDir = GetDefaultThemeDir();
		//Plik MP3 z domyslnej kompozycji
		if(FileExists(ThemeDir+"\\\\\Sound\\\\In.mp3"))
			return ThemeDir+"\\\\\Sound\\\\In.mp3";
		//Plik WAV z domyslnej kompozycji
		else if(FileExists(ThemeDir+"\\\\\Sound\\\\In.mp3"))
			return ThemeDir+"\\\\\Sound\\\\In.mp3";
	}
	return "";
}
//---------------------------------------------------------------------------

//Hook na aktwyna zakladke lub okno rozmowy
INT_PTR __stdcall OnActiveTab(WPARAM wParam, LPARAM lParam)
{
	//Pobieranie danych kontaktku
	TPluginContact ActiveTabContact = *(PPluginContact)lParam;
	UnicodeString JID = (wchar_t*)ActiveTabContact.JID;
	UnicodeString Res = (wchar_t*)ActiveTabContact.Resource;
	if(!Res.IsEmpty()) Res = "/" + Res;
	if(ActiveTabContact.IsChat)
	{
		JID = "ischat_" + JID;
		Res = "";
	}
	UnicodeString UserIdx = ":" + IntToStr(ActiveTabContact.UserIdx);
	//Zakladka z kontaktem nie byla jeszcze otwarta
	if(TabsList->IndexOf(JID+Res+UserIdx)==-1)
		//Dodawanie JID do tablicy zakladek
		TabsList->Add(JID+Res+UserIdx);

	return 0;
}
//---------------------------------------------------------------------------

//Hook na wylaczenie auto-oddalania
INT_PTR __stdcall OnAutoAwayOff(WPARAM wParam, LPARAM lParam)
{
	//Odznaczenie wylaczenia trybu auto-oddalenia
	AutoAwayMode = false;

	return 0;
}
//---------------------------------------------------------------------------

//Hook na wlaczenie auto-oddalania
INT_PTR __stdcall OnAutoAwayOn(WPARAM wParam, LPARAM lParam)
{
	//Odznaczenie wlaczenia trybu auto-oddalenia
	AutoAwayMode = true;

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zamkniecie okna rozmowy lub zakladki
INT_PTR __stdcall OnCloseTab(WPARAM wParam, LPARAM lParam)
{
	//Pobieranie danych dt. kontaktu
	TPluginContact CloseTabContact = *(PPluginContact)lParam;
	UnicodeString JID = (wchar_t*)CloseTabContact.JID;
	UnicodeString Res = (wchar_t*)CloseTabContact.Resource;
	if(!Res.IsEmpty()) Res = "/" + Res;
	if(CloseTabContact.IsChat)
	{
		JID = "ischat_" + JID;
		Res = "";
	}
	UnicodeString UserIdx = ":" + IntToStr(CloseTabContact.UserIdx);
	//Usuwanie JID z listy aktywnych zakladek
	if(TabsList->IndexOf(JID+Res+UserIdx)!=-1)
		TabsList->Delete(TabsList->IndexOf(JID+Res+UserIdx));
	//Usuwanie JID z listy kontaktow od ktorych otrzymano wiadomosc
	if(MsgsList->IndexOf(JID+Res+UserIdx)!=-1)
		MsgsList->Delete(MsgsList->IndexOf(JID+Res+UserIdx));

	return 0;
}
//---------------------------------------------------------------------------

//Pobieranie listy wszystkich otartych zakladek/okien
INT_PTR __stdcall OnFetchAllTabs(WPARAM wParam, LPARAM lParam)
{
	//Uchwyt do okna rozmowy nie zostal jeszcze pobrany
	if(!hFrmSend)
		//Przypisanie uchwytu okna rozmowy
		hFrmSend = (HWND)(int)wParam;
	//Pobieranie danych kontatku
	TPluginContact FetchAllTabsContact = *(PPluginContact)lParam;
	UnicodeString JID = (wchar_t*)FetchAllTabsContact.JID;
	UnicodeString Res = (wchar_t*)FetchAllTabsContact.Resource;
	if(!Res.IsEmpty()) Res = "/" + Res;
	if(FetchAllTabsContact.IsChat)
	{
		JID = "ischat_" + JID;
		Res = "";
	}
	UnicodeString UserIdx = ":" + IntToStr(FetchAllTabsContact.UserIdx);
	//Dodawanie JID do listy otwartych zakladek
	if(TabsList->IndexOf(JID+Res+UserIdx)==-1)
		TabsList->Add(JID+Res+UserIdx);
	//Dodawanie JID z listy kontaktow od ktorych otrzymano wiadomosc
	if(MsgsList->IndexOf(JID+Res+UserIdx)==-1)
		MsgsList->Add(JID+Res+UserIdx);

	return 0;
}
//---------------------------------------------------------------------------

//Hook na odtwarzanie dzwiekow
INT_PTR __stdcall OnPlaySound(WPARAM wParam, LPARAM lParam)
{
	//Dzwiek nowej wiadomosci
	if((lParam==SOUND_FIRSTIN)||(lParam==SOUND_IN))
		//Blokada
		return 1;

	return 0;
}
//---------------------------------------------------------------------------

//Hook na aktywna zakladke
INT_PTR __stdcall OnPrimaryTab(WPARAM wParam, LPARAM lParam)
{
	//Uchwyt do okna rozmowy nie zostal jeszcze pobrany
	if(!hFrmSend)
		//Przypisanie uchwytu okna rozmowy
		hFrmSend = (HWND)(int)wParam;

	return 0;
}
//---------------------------------------------------------------------------

//Hook na odbieranie wiadomosci
INT_PTR __stdcall OnRecvMsg(WPARAM wParam, LPARAM lParam)
{
	//Pobieranie danych wiadomosci
	TPluginMessage RecvMsgMessage = *(PPluginMessage)lParam;
	//Rodzaj wiadomosci
	if(RecvMsgMessage.Kind!=MSGKIND_RTT)
	{
		//Wiadomosc nie jest pusta
		if(!((UnicodeString)((wchar_t*)RecvMsgMessage.Body)).IsEmpty())
		{
			//Pobieranie danych kontaktu
			TPluginContact RecvMsgContact = *(PPluginContact)wParam;
			UnicodeString JID = (wchar_t*)RecvMsgContact.JID;
			UnicodeString Res = (wchar_t*)RecvMsgContact.Resource;
			if(!Res.IsEmpty()) Res = "/" + Res;
			if(RecvMsgContact.IsChat)
			{
				JID = "ischat_" + JID;
				Res = "";
			}
			UnicodeString UserIdx = ":" + IntToStr(RecvMsgContact.UserIdx);
			//Pierwsza wiadomosc od kontatku
			bool FirstMsg = false;
			if(MsgsList->IndexOf(JID+Res+UserIdx)==-1)
			{
				MsgsList->Add(JID+Res+UserIdx);
				FirstMsg = true;
			}
			//Dzwieki sa wlaczone
			if(ChkSoundEnabled())
			{
				//Zakladka z kontaktem nie jest otwarta || okno rozmowy jest nieaktywne || wlaczony tryb auto-oddalenia
				if((TabsList->IndexOf(JID+Res+UserIdx)==-1)||(hFrmSend!=GetForegroundWindow())||(AutoAwayMode))
				{
					//Odtworzenia dzwieku przychodzacej pierwszej wiadomosci
					if(FirstMsg) PluginLink.CallService(AQQ_SYSTEM_PLAYSOUND,(WPARAM)GetFirstSoundPath().w_str(),2);
					//Odtworzenia dzwieku przychodzacej wiadomosci
					else PluginLink.CallService(AQQ_SYSTEM_PLAYSOUND,(WPARAM)GetInSoundPath().w_str(),2);
				}
			}
		}
	}

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zamkniecie/otwarcie okien
INT_PTR __stdcall OnWindowEvent(WPARAM wParam, LPARAM lParam)
{
	//Pobranie informacji o oknie i eventcie
	TPluginWindowEvent WindowEvent = *(PPluginWindowEvent)lParam;
	int Event = WindowEvent.WindowEvent;
	UnicodeString ClassName = (wchar_t*)WindowEvent.ClassName;

	//Otwarcie okna rozmowy
	if((ClassName=="TfrmSend")&&(Event==WINDOW_EVENT_CREATE))
		//Przypisanie uchwytu do okna rozmowy
		hFrmSend = (HWND)WindowEvent.Handle;
	//Zamkniecie okna rozmowy
	if((ClassName=="TfrmSend")&&(Event==WINDOW_EVENT_CLOSE))
		//Usuniecie uchwytu do okna rozmowy
		hFrmSend = NULL;

	return 0;
}
//---------------------------------------------------------------------------

extern "C" INT_PTR __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
	//Linkowanie wtyczki z komunikatorem
	PluginLink = *Link;
	//Hook na aktwyna zakladke lub okno rozmowy
	PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_ACTIVETAB,OnActiveTab);
	//Hook na wylaczenie auto-oddalania
	PluginLink.HookEvent(AQQ_SYSTEM_AUTOMATION_AUTOAWAY_OFF,OnAutoAwayOff);
	//Hook na wlaczenie auto-oddalania
	PluginLink.HookEvent(AQQ_SYSTEM_AUTOMATION_AUTOAWAY_ON,OnAutoAwayOn);
	//Hook na zamkniecie okna rozmowy lub zakladki
	PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_CLOSETAB,OnCloseTab);
	//Hook na odtwarzanie dzwiekow
	PluginLink.HookEvent(AQQ_SYSTEM_PLAYSOUND,OnPlaySound);
	//Hook na odbieranie nowej wiadomosci
	PluginLink.HookEvent(AQQ_CONTACTS_RECVMSG,OnRecvMsg);
	//Hook na zamkniecie/otwarcie okien
	PluginLink.HookEvent(AQQ_SYSTEM_WINDOWEVENT,OnWindowEvent);
	//Wszystkie moduly zostaly zaladowane
	if(PluginLink.CallService(AQQ_SYSTEM_MODULESLOADED,0,0))
	{
		//Hook na pobieranie aktywnych zakladek
		PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_FETCHALLTABS,OnFetchAllTabs);
		PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_PRIMARYTAB,OnPrimaryTab);
		//Pobieranie aktywnych zakladek
		PluginLink.CallService(AQQ_CONTACTS_BUDDY_FETCHALLTABS,0,0);
		//Usuniecie hooka na pobieranie aktywnych zakladek
		PluginLink.UnhookEvent(OnPrimaryTab);
		PluginLink.UnhookEvent(OnFetchAllTabs);
	}

	return 0;
}
//---------------------------------------------------------------------------

extern "C" INT_PTR __declspec(dllexport) __stdcall Unload()
{
	//Wyladowanie wszystkich hookow
	PluginLink.UnhookEvent(OnActiveTab);
	PluginLink.UnhookEvent(OnAutoAwayOff);
	PluginLink.UnhookEvent(OnAutoAwayOn);
	PluginLink.UnhookEvent(OnCloseTab);
	PluginLink.UnhookEvent(OnPlaySound);
	PluginLink.UnhookEvent(OnRecvMsg);
	PluginLink.UnhookEvent(OnWindowEvent);

	return 0;
}
//---------------------------------------------------------------------------

//Informacje o wtyczce
extern "C" PPluginInfo __declspec(dllexport) __stdcall AQQPluginInfo(DWORD AQQVersion)
{
	PluginInfo.cbSize = sizeof(TPluginInfo);
	PluginInfo.ShortName = L"BetterMsgSound";
	PluginInfo.Version = PLUGIN_MAKE_VERSION(1,1,0,0);
	PluginInfo.Description = L"Udoskonala dŸwiêkow¹ informacjê o nowej wiadomoœci poprzez odtwarzanie jej tylko gdy okno rozmowy bêdzie nieaktywne lub zak³adka z kontaktem nie bêdzie otwarta oraz kiedy bêdziemy nieaktywni.";
	PluginInfo.Author = L"Krzysztof Grochocki";
	PluginInfo.AuthorMail = L"kontakt@beherit.pl";
	PluginInfo.Copyright = L"Krzysztof Grochocki";
	PluginInfo.Homepage = L"http://beherit.pl";
	PluginInfo.Flag = 0;
	PluginInfo.ReplaceDefaultModule = 0;

	return &PluginInfo;
}
//---------------------------------------------------------------------------
