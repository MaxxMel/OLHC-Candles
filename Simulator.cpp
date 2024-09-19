#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <sstream>
#include <cmath>
#include <numeric>
#include <chrono>
#include <unordered_map>

using namespace std; 
using namespace std::chrono;


struct Trade
{
    uint64_t local_timestamp; 
    bool side; // 1 - buy 
    double price;
    int amount;
};  
struct LOB
{
    uint64_t local_timestamp; 
    int ask_amount; 
    int bid_amount; 
    double ask_price; 
    double bid_price; 
    
    double Bid_ask_spread; 
    int Imbalance; 

};

Trade parseStringTrade(const string& input) {
    Trade trade;
    istringstream iss(input);
    string token;
    if (getline(iss, token, ',')) {
        trade.local_timestamp = stoull(token); 
    }

    
    if (getline(iss, token, ',')) {
        if (token == "buy") {
            trade.side = true;  
        } else {
            trade.side = false; 
        }
    }

    if (getline(iss, token, ',')) {
        trade.price = stod(token); 
    }

    if (getline(iss, token, ',')) {
        trade.amount = stoi(token);
    }

    return trade;
}

LOB parseStringLOB(const string& input)
{
    LOB lob; 
    istringstream iss(input); 
    string token; 
    if (getline(iss, token, ',')){
        lob.local_timestamp = stoull(token); 
    }
    if (getline(iss, token, ',')){
        lob.ask_amount = stod(token); 
    }
    if (getline(iss, token, ',')){
        lob.ask_price = stod(token); 
    }
    if (getline(iss, token, ',')){
        lob.bid_price = stod(token); 
    }
    if (getline(iss, token, ',')){
        lob.bid_amount = stod(token); 
    }
    lob.Bid_ask_spread = lob.ask_price - lob.bid_price;
    lob.Imbalance = lob.ask_amount - lob.bid_amount; 
    return lob;   
}

vector <Trade> GetTradeData(string instrument)
{
    vector <Trade> v; 
    filesystem::path file_path = instrument;
    filesystem::directory_entry entry(file_path);
    
    ifstream data_file(entry.path()); 
    string line;

    getline(data_file, line);
 
    while (getline(data_file, line))
        {
            Trade T = parseStringTrade(line);
            v.push_back(T);    
        }
    return v;     
}
vector <LOB> GetLOBData(string instrument)
{
    vector <LOB> v; 
    filesystem::path file_path = instrument;
    filesystem::directory_entry entry(file_path);
    
    ifstream data_file(entry.path()); 
    string line;

    getline(data_file, line);
 
    while (getline(data_file, line))
        {
            LOB T = parseStringLOB(line);
            v.push_back(T);    
        }
    return v;     
} 

struct CombinedEntry
{
    uint64_t local_timestamp;
    bool is_trade;  // true - Trade, false - LOB

    Trade trade;
    LOB lob;

    CombinedEntry(const Trade& t)
        : local_timestamp(t.local_timestamp), is_trade(true), trade(t) {}

    CombinedEntry(const LOB& l)
        : local_timestamp(l.local_timestamp), is_trade(false), lob(l) {}
};

vector<CombinedEntry> CombineAndSortData(const vector<Trade>& trades, const vector<LOB>& lobs)
{
    vector<CombinedEntry> combined;

    // Добавляем все элементы из trades и lobs в общий вектор
    for (const auto& trade : trades) {
        combined.emplace_back(trade);
    }

    for (const auto& lob : lobs) {
        combined.emplace_back(lob);
    }

    // Сортируем по local_timestamp
    sort(combined.begin(), combined.end(), [](const CombinedEntry& a, const CombinedEntry& b) {
        return a.local_timestamp < b.local_timestamp;
    });

    return combined;
}
struct InstrumentData
{
    vector <Trade> Trades; 
    vector <LOB> LOBs; 
    vector <CombinedEntry> fullSortedData; 

    InstrumentData(string& pathToTrades, string& pathToLOBs)
    {
        this->Trades = GetTradeData(pathToTrades);
        this->LOBs = GetLOBData(pathToLOBs); 
        this->fullSortedData = CombineAndSortData(Trades, LOBs);
    }
};

class Candle
{
    public: 
    uint64_t open_time;   // Время открытия свечи
    uint64_t T;    // Длительность свечи
    double open_price;    // Цена открытия
    double high_price;    // Максимальная цена
    double low_price;     // Минимальная цена
    double close_price;   // Цена закрытия
    double avg_buy_price; // Средняя цена покупок
    double avg_sell_price;// Средняя цена продаж
    uint64_t buy_amount;    // Объем покупок
    uint64_t sell_amount;   // Объем продаж

    uint64_t expected_close_time;
    bool isFinished = false; 
    bool first_trade_found ;

    int buy_trade_count ;
    int sell_trade_count ;

    double buys_sum_money ; 
    double sell_sum_money ; 
    Trade last_trade; 

    Candle(uint64_t ot, uint64_t T_)
        : open_time(ot), T(T_), open_price(0), high_price(0), low_price(0), close_price(0),
          avg_buy_price(0), avg_sell_price(0), buy_amount(0), sell_amount(0),    buy_trade_count(0),
          sell_trade_count(0), buys_sum_money(0), sell_sum_money(0),  first_trade_found(false)

          {expected_close_time = open_time + T;}

    void editCandleByTrade(const Trade& data_about_trade)
    {
        if (data_about_trade.local_timestamp <= expected_close_time)// && isFinished)
        {
        
        // Инициализация цены открытия, если это первая сделка в свече
        if (!first_trade_found) {
            open_price = data_about_trade.price;
            high_price = data_about_trade.price;
            low_price = data_about_trade.price;
            close_price = data_about_trade.price; // В начале close_price совпадает с open_price
            first_trade_found = true;
            
        } else {
            // Обновление high и low цен
            if (data_about_trade.price > high_price) high_price = data_about_trade.price;
            if (data_about_trade.price < low_price) low_price = data_about_trade.price;

            // Обновление цены закрытия
            close_price = data_about_trade.price;
        }

        // Обновление объёмов и средних цен
        if (data_about_trade.side) {
            // Если это покупка
            buy_trade_count++;
            buys_sum_money += data_about_trade.price;

            buy_amount += data_about_trade.amount;

            avg_buy_price = buys_sum_money / buy_trade_count;
            //avg_buy_price += data_about_trade.price * data_about_trade.amount;  
             
        } else {
               // ПОФИКСИТЬ avg
            // Если это продажа
            sell_trade_count++;
            sell_sum_money += data_about_trade.price; 

            sell_amount += data_about_trade.amount;
            avg_sell_price = sell_sum_money / sell_trade_count; 
            //avg_sell_price += data_about_trade.price * data_about_trade.amount;
        }
        }else 
        {
            this->isFinished = true;
            //this->close_price = data_about_trade.price;
            this->last_trade = data_about_trade;
            return;
        }

    
    }

    void ShowInformation() const {
        cout << "Open price: " << open_price << "\n";
        cout << "High price: " << high_price << "\n";
        cout << "Low price: " << low_price << "\n";
        cout << "Close price: " << close_price << "\n";
        cout << "Average buy price: " << avg_buy_price << "\n";
        cout << "Average sell price: " << avg_sell_price << "\n";
        cout << "Buy amount: " << buy_amount << "\n";
        cout << "Sell amount: " << sell_amount << "\n";
    }
    
}; 

struct knownLobHistory
{
    vector<LOB> last_lobs;
    vector<double> spreads;
    vector<double> mid_bid_amount;
    vector<double> mid_ask_amount;
    double total_ask_amount = 0;  // Сумма ask_amount всех объектов в last_lobs
    double total_bid_amount = 0;  // Сумма bid_amount всех объектов в last_lobs

    void update(const LOB& obj)
    {
        // Добавляем новый LOB в историю
        last_lobs.push_back(obj);

        // Обновляем спред
        spreads.push_back(obj.ask_price - obj.bid_price);

        // Обновляем суммы ask и bid
        total_ask_amount += obj.ask_amount;
        total_bid_amount += obj.bid_amount;

        // Рассчитываем средние значения и добавляем их в соответствующие векторы
        int num_lobs = last_lobs.size();  // Количество элементов в истории

        mid_ask_amount.push_back(total_ask_amount / num_lobs);
        mid_bid_amount.push_back(total_bid_amount / num_lobs);
    }
    
    void AnalyzeKnownLobDataAndMakeDecision() // Анализ инфы и принятие решения покупать сейчас или нет
    {
        int period (7); 
        if (MidBidAmountIsGrow(mid_bid_amount, period) && MidAskAmountIsGrow(mid_ask_amount, period)) // логика есл и растет объем и ask и bid // рынок стабилен // цена сильно не поменяется 
            {}
        else if (MidBidAmountIsDrop(mid_bid_amount, period) && MidAskAmountIsDrop(mid_ask_amount, period)) // объяемы падают, рынок не стабилен
            {
                // Анализ цен заполнить логику 
            }
        else if (MidAskAmountIsDrop(mid_ask_amount, period) && MidBidAmountIsGrow(mid_bid_amount, period)) // объемы продаж падают а покупок растут => цена пойдет наверх
            {
                // покупаем сейчас или по цене закрытия  заполнить логику
            }
        else if (MidAskAmountIsGrow(mid_ask_amount, period) && MidBidAmountIsDrop(mid_bid_amount, period)) // объемы продаж растут а покупок падают => цена пойдет вних 
            {
                // продаем сейчас или по цене закрытрия заполнить логику
            }
    
    }
};


bool MidBidAmountIsDrop(vector<double> Amid_bid_amount, int period)
{
     if (Amid_bid_amount.size() < 7) return false; 

    // Проверка на падение среднего объема покупок за последние 7 сделок
    bool drop_detected = true;
    for (int i = Amid_bid_amount.size() - 7; i < Amid_bid_amount.size() - 1; ++i) 
    {
        if (Amid_bid_amount[i] <= Amid_bid_amount[i+1]) {
            drop_detected = false;
            break; 
        }
    }

    return drop_detected; 
}
bool MidAskAmountIsDrop(vector<double> Amid_ask_amount, int period)
{
    if (Amid_ask_amount.size() < 7) return false; 

    // Проверка на падение среднего объема покупок за последние 7 сделок
    bool drop_detected = true;
    for (int i = Amid_ask_amount.size() - 7; i < Amid_ask_amount.size() - 1; ++i) 
    {
        if (Amid_ask_amount[i] <= Amid_ask_amount[i+1]) {
            drop_detected = false;
            break; 
        }
    }

    return drop_detected; 
}

bool MidAskAmountIsGrow(vector<double> Amid_ask_amount, int period)
{
    if (Amid_ask_amount.size() < period)
    {
        cerr << "Недостаточно данных для анализа!" << endl;
        exit(3);  // return false;
    }
    double prev_sum = 0;
    double current_sum = 0;
    // Суммируем значения до текущего периода
    for (int i = Amid_ask_amount.size() - period - 1; i < Amid_ask_amount.size() - 1; i++) {
        prev_sum += Amid_ask_amount[i];
    }

    // Суммируем значения за текущий период
    for (int i = Amid_ask_amount.size() - period; i < Amid_ask_amount.size(); i++) {
        current_sum += Amid_ask_amount[i];
    }
    double prev_avg = prev_sum / period;
    double current_avg = current_sum / period;

    if (current_avg > prev_avg) {
        return true;
    } else {
        return false;         
    }
}
bool MidBidAmountIsGrow(vector<double> Amid_bid_amount, int period)
{
    if (Amid_bid_amount.size() < period)
    {
        cerr << "Недостаточно данных для анализа!" << endl;
        exit(3); // return false
    }
    double prev_sum = 0;
    double current_sum = 0;
    // Суммируем значения до текущего периода
    for (int i = Amid_bid_amount.size() - period - 1; i < Amid_bid_amount.size() - 1; i++) {
        prev_sum += Amid_bid_amount[i];
    }

    // Суммируем значения за текущий период
    for (int i = Amid_bid_amount.size() - period; i < Amid_bid_amount.size(); i++) {
        current_sum += Amid_bid_amount[i];
    }
    double prev_avg = prev_sum / period;
    double current_avg = current_sum / period;

    if (current_avg > prev_avg) {
        return true;
    } else {
        return false;         
    }
}



class Simulator 
{
    public: 
    vector<int> actions;


    uint64_t T; 
    InstrumentData ID; 
    vector<Candle> CandlesHistory; 
    vector<uint64_t> Candles_Boarder_Timestamps; 
    int builded_Candles_num = 0; 
    knownLobHistory orderbook; 

    Simulator(uint64_t T, InstrumentData& ID_) : T(T), ID(ID_)//, actions(actions_)
    {
        //if (actions.size() != (ID.Trades.back().local_timestamp - ID.Trades[0].local_timestamp) / T + 1) cerr << "Data is not correnct" << endl;  
    }

    Candle& CurCandleOf(vector <Candle>& CandlesHistory)
    {
        return CandlesHistory.back();
    }
    
    void IterateData()
    {
        for (auto current_comb : ID.fullSortedData)
        {
            if (current_comb.is_trade)
            {
                
                if (CandlesHistory.empty()) 
                {
                    CandlesHistory.push_back(Candle(current_comb.local_timestamp, T)); 
                    CurCandleOf(CandlesHistory).editCandleByTrade(current_comb.trade); 
                }
                else if (CurCandleOf(CandlesHistory).isFinished == true)
                {
                    CandlesHistory.push_back(Candle(CurCandleOf(CandlesHistory).expected_close_time, T)); 
                    CurCandleOf(CandlesHistory).editCandleByTrade(CandlesHistory[CandlesHistory.size() - 2].last_trade); 
                    CurCandleOf(CandlesHistory).editCandleByTrade(current_comb.trade); 
                } else
                {
                    CurCandleOf(CandlesHistory).editCandleByTrade(current_comb.trade); 
                    if (CurCandleOf(CandlesHistory).isFinished) Candles_Boarder_Timestamps.push_back(CurCandleOf(CandlesHistory).expected_close_time); 
                }
            } else // обработка если это LOB
            {
                LOB curLobData = current_comb.lob; 
                orderbook.update(curLobData); 
                
            }
            
        }
    }
    
}; 




int main()
{
    string path_Trade = R"(C:\Users\Maxim\OneDrive\Рабочий стол\CMF\Task1\trades_dogeusdt.csv)"; 
    string path_LOB = R"(C:\Users\Maxim\OneDrive\Рабочий стол\CMF\Task1\bbo_dogeusdt.csv)";

    InstrumentData ID0(path_Trade, path_LOB);  
    Simulator sim(3232324343, ID0); 
   
    sim.IterateData(); 
    for(auto it : sim.CandlesHistory)
    {
    it.ShowInformation(); 
        cout << '\n'; 
    }

    return 0; 
}
