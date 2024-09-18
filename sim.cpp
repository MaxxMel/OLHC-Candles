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
            isFinished = true;
            close_price = data_about_trade.price;
            last_trade = data_about_trade;
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

struct LOB_buffer
    {
        vector<LOB> Last_Lobs;
        int buf_size = 5; 

        void upload(const LOB& obj)
        {
            if (Last_Lobs.size() < buf_size) Last_Lobs.push_back(obj); 
            else
            {
                vector<LOB> temp; 
                for (int i(1); i < buf_size; i++) temp.push_back(Last_Lobs[i]); 
                temp.push_back(obj); 
                this->Last_Lobs = temp;
            }
        }
    }; 
    
class Simulator 
{
    public: 
    vector<int> actions;


    uint64_t T; 
    InstrumentData ID; 
    vector<Candle> CandlesHistory; 
    vector<uint64_t> Candles_Boarders; 
    int builded_Candles_num = 0; 


    Simulator(uint64_t T, InstrumentData& ID_) : T(T), ID(ID_) 
    {}

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
                Trade current_trade = current_comb.trade; 
                if (CandlesHistory.empty()) 
                {
                    CandlesHistory.push_back(Candle(current_comb.local_timestamp, T)); 
                    CurCandleOf(CandlesHistory).editCandleByTrade(current_trade); 
                }
                else if (CurCandleOf(CandlesHistory).isFinished == true)
                {
                    CandlesHistory.push_back(Candle(CurCandleOf(CandlesHistory).expected_close_time, T)); 
                    CurCandleOf(CandlesHistory).editCandleByTrade(CandlesHistory[CandlesHistory.size() - 2].last_trade); 
                    CurCandleOf(CandlesHistory).editCandleByTrade(current_trade); 
                } else
                {
                    CurCandleOf(CandlesHistory).editCandleByTrade(current_trade); 
                }
            } else // обработка если это LOB
            {
                
            }
            
        }
    } 
    
}; 




int main()
{
    string path_Trade = R"(C:\Users\Maxim\OneDrive\Рабочий стол\CMF\Task1\trades_dogeusdt.csv)"; 
    string path_LOB = R"(C:\Users\Maxim\OneDrive\Рабочий стол\CMF\Task1\bbo_dogeusdt.csv)";

    InstrumentData ID0(path_Trade, path_LOB);  
    Simulator sim(323232, ID0); 
    sim.IterateData(); 
    for(auto it : sim.CandlesHistory)
    {
    it.ShowInformation(); 
        cout << '\n'; 
    }
    for (int i(0); i < sim.CandlesHistory.size(); ++i)
    {
        if (sim.CandlesHistory[i].close_price != sim.CandlesHistory[i + 1].open_price)
        {
            cout << "fuck";
            break; 
        }
    }
    Candle c(1723248002468088, 3232332);


    
    return 0; 
}
