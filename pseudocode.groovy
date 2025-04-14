// TODO: usunąć ten edge case z ackami bo nie trzeba jak lamport, napisać jawnie o lamporcie, zrobić tę metodę przeskakiwania w kolejce na bazie liczby requestów na danej pozycji i własnego id, zamienić broadcasty otagowane na broadcasty z parametrem bo są cięższe do obsłużenia

state IDLE, TABLE_SEEK, TABLE_WAIT, TABLE_PLAY

signal TABLE_REQ<tid>,			// prośba o dostanie się do stołu tid			| broadcast
	TABLE_FREE,					// zgoda na zapytanie o stół					| odpowiedź
	TABLE_OCCUPIED,				// odmowa na zapytanie o stół					| odpowiedź
	GAME_BGN_REQ<vote>#tid,		// prośba (+ głos) o rozpoczęcie gry przy stole	| broadcast otagowany
	GAME_BGN_ACK<vote,			// zgoda na zapytanie o rozpoczęcie	(+ głos)	| odpowiedź
	GAME_END_REQ#tid, 			// prośba o zakończenie gry przy stole			| broadcast otagowany
	GAME_END_ACK				// zgoda na zapytanie o koniec gry				| odpowiedź
	GAME_OVER<tid>				// broadcast że gra się zakończyła na stole, pobudka żeby szukać stołu jak ktoś stał

const PLAYER_NUM,				// liczba graczy
	TABLE_NUM,					// liczba stołów
	PLAYERS_NEEDED				// gracze potrzebni do gry

state BASE {

	table_id: int							// trzymana informacja w procesie o trzymanym stole
	table_activity[TABLE_NUM]: array[bool]	// szacunkowa liczba osób, które pytają o stół
	games_played: int						// liczba granych gier, jako priorytet

	signal {
		case TABLE_REQ<tid>:
			table_activity[tid]++
		case GAME_OVER<tid>:
			table_activity[tid] = 0
	}

	// znajdź stół który jest jak najbliższy wypełnienia
	// ale nie jest pełny
	choose_table() {
		tid: int = -1
		max: int = -inf
		for t in table_activity:
			if table_activity[t] >= PLAYERS_NEEDED:		// stół jest pełny
				continue
			if table_activity[t] > max:					// stół jest bliższy pełnego, taki preferujemy
				tid = t
				max = table_activity[t]
		return tid
	}

}

state IDLE {

	signal {
		case TABLE_REQ<tid>:
			send(TABLE_FREE)							// jak ktoś chce jakiś stół to ok, nie ma problemu, to nas nie dotyczy
							
		case GAME_OVER:
			logic()										// pobudka :)
			
												
	}

	logic {
		wait(random)									// czekaj losową ilość czasu
		goto(TABLE_SEEK)								// zacznij szukać stołu
	}
	
}

state TABLE_SEEK {

	answers = 0
	acks = 0							// ilość zebranych zgód na dojście do stołu
	naks = 0
	received_acks : set

	signal {
		case TABLE_FREE:
				received_acks[tid] += sender
				answers++
				acks++						// zgoda, zwiększ licznik


		case TABLE_OCCUPIED:
			answers++
			naks++
		case TABLE_REQ<tid>:
			if older(timestamp,games_played,id):		// sprawdź czy własny znacznik czasowy jest mniejszy, rozwiąż konflikty id
				send(TABLE_OCCUPIED)	// nasz request jest starszy, odrzucamy ten przychodzący
				
			else:
				if sender in received_acks[tid]:  //sprawdzamt, czy nie orzymaliśmy wcześniej od tego staruszka zgody na gre, jeżeli on też zmienił zdanie, to zmieniamy jego poprzednią ocene na brak zgody
					acks --
					naks++
				send(TABLE_FREE)
				
	}

	logic {
		received_acks = {}
		table_id = choose_table()						// wybierz losowy stół
		if table_id == -1:
			goto(IDLE)									// nie ma wolnego stołu, czekaj

		broadcast(TABLE_REQ<table_id>)					// ogłoś globalnie prośbę o stół
		wait until (answers == PLAYERS_NUM)				// czekaj na wszystkie zgody pozytywne

		if naks < 4:
			goto(TABLE_WAIT)							// jesteś wpuszczony, czekaj przy stole
		else:
			goto(IDLE)									// brak zgody, porzuć stan i zacznij od nowa
	}
}

state TABLE_WAIT {

	ready = 0											// gracze gotowi do gry
	votes = [0,0,...]		// liczba głosów na dane gry

	signal {
		case TABLE_REQ<tid>:
			if table_id != tid:
				send(TABLE_FREE)							// ktoś pyta o inny stół, nas nie obchodzi
			else:
				send(TABLE_OCCUPIED)							// ktoś chce wejść, nie ma miejsca, nie ok
		case GAME_BGN_REQ<vote>#table_id:
			votes[vote]++								// Odnotuj głos na grę
			send(GAME_BGN_ACK)							// ktoś pyta się o gotowość, odpowiedz że tak
			if older(req_timestamp, timestamp):			// TYLKO jeśli ten ktoś dołączył PO nas, anty-duplikat
				ready++									// jeśli ktoś prosi to jest logicznie gotowy, zwiększ licznik
		case GAME_BGN_ACK<vote>:
			ready++										// ktoś potwierdził gotowość, zwiększ licznik
	}

	logic {
		vote = random(games)
		req_timestamp = broadcast(GAME_BGN_REQ<vote>#table_id)	// zgłoś gotowość na stole
		wait until (ready == PLAYERS_NEEDED)					// czekaj aż wszyscy gotowi
		winning_vote = max(vote,id)								// na bazie głosów (lub id) wybierz zwycięzcę
		goto(TABLE_PLAY)										// graj
	}
}

signal GAME_BGN<vote>								// dwukierunkowy sygnał gotowości do gry     | broadcast otagowany / odpowiedź

state TABLE_WAIT_ALT {

	ready = 0											// gracze gotowi do gry
	votes = [0,0,...]									// liczba głosów na dane gry
	players = {}										// zbiór znanych graczy

	signal {
		case TABLE_REQ<tid>:
			if table_id != tid:
				send(TABLE_FREE)							// ktoś pyta o inny stół, nas nie obchodzi
			else:
				send(TABLE_OCCUPIED)							// ktoś chce wejść, nie ma miejsca, nie ok
		case GAME_BGN<vote>#table_id:
			if id not in players:
				send(GAME_BGN<vote>)					// nowo poznany gracz, przekaż gotowość + głos
				players += id							// gracz poznany
				votes[vote]++							// Odnotuj głos na grę
				ready++									// zwiększ licznik gotowości
	}

	logic {
		vote = random(games)
		req_timestamp = broadcast(GAME_BGN<vote>#table_id)		// zgłoś gotowość na stole
		wait until (ready == PLAYERS_NEEDED)					// czekaj aż wszyscy gotowi
		winning_vote = max(vote,id)								// na bazie głosów (lub id) wybierz zwycięzcę
		goto(TABLE_PLAY)										// graj
	}
}

state TABLE_PLAY {

	game_over = 0										// licznik graczy gotowych do zakończenia gry

	signal {
		case TABLE_REQ<tid>:
			if table_id != tid:
				send(TABLE_FREE)							// ktoś pyta o inny stół, nas nie obchodzi
			else:
				send(TABLE_OCCUPIED)							// ktoś chce wejść, gra trwa więc nie ok
		case GAME_END_REQ#table_id:
			send(GAME_END_ACK)							// ktoś pyta się o zakończenie, odpowiedz że tak
		case GAME_END_ACK:
			game_over++									// ktoś inny jest gotowy zakończyć grę, zwiększ licznik
	}

	logic {
		wait(random)									// symulacja grania pewien czas
		broadcast(GAME_END_REQ#table_id)				// nadaj chęć zakończenia gry
		wait until (game_over == PLAYERS_NEEDED)		// czekaj aż wszyscy będą gotowi
		table_id = null									// opuść stół
		goto(IDLE)										// wróć na start

		games_played++
		broadcast(GAME_OVER)
	}
}
