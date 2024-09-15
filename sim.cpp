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
    bool side; 
    double price;
    int amount;
};  

void GetTradeData(string instrument, vector<Trade>& v);

struct LOB
{
    uint64_t local_timestamp; 
    int ask_amount; 
    int bid_amount; 
    double ask_price; 
    double bid_price; 
};



vector <Trade> Trades; 
uint64_t minTime = UINT64_MAX;
uint64_t maxTime = 0;
uint64_t timeWindow  = maxTime - minTime; 



Trade parseString(const string& input) {
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

class Candle {
    public: 
    uint64_t open_time;   // Время открытия свечи
    uint64_t duration;    // Длительность свечи
    double open_price;    // Цена открытия
    double high_price;    // Максимальная цена
    double low_price;     // Минимальная цена
    double close_price;   // Цена закрытия
    double avg_buy_price; // Средняя цена покупок
    double avg_sell_price;// Средняя цена продаж
    uint64_t buy_amount;    // Объем покупок
    uint64_t sell_amount;   // Объем продаж
    
    
    Candle(uint64_t t, uint64_t d, vector<Trade>& TradeDataBase)
        : open_time(t), duration(d), open_price(0), high_price(0), low_price(0), close_price(0),
          avg_buy_price(0), avg_sell_price(0), buy_amount(0), sell_amount(0) {

        uint64_t close_time = open_time + duration;
        bool first_trade_found = false; 

        int buy_trade_count = 0;
        int sell_trade_count = 0;

        for (const auto& it : TradeDataBase) {
           
            if (it.local_timestamp >= open_time && it.local_timestamp <= close_time) {
                
                if (!first_trade_found) {
                    open_price = it.price;
                    low_price = it.price;  
                    high_price = it.price; 
                    first_trade_found = true;
                }

              
                if (it.price > high_price) high_price = it.price;
                if (it.price < low_price) low_price = it.price;

                
                close_price = it.price;

             
                if (it.side) {
                    // Покупка
                    buy_amount += it.amount;
                    avg_buy_price += it.price * it.amount;
                    buy_trade_count++;
                } else {
                    //Продажа
                    sell_amount += it.amount;
                    avg_sell_price += it.price * it.amount;
                    sell_trade_count++;
                }
             
            }
        }

        
        if (buy_trade_count > 0) avg_buy_price /= buy_amount;
        if (sell_trade_count > 0) avg_sell_price /= sell_amount;
    }

    void ShowInformation() {
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
vector<Trade> GetTradeData(string instrument); 

void CountMinAndMaxTime(vector<Trade>& data, uint64_t& TW)
{
    for (const auto it : data)
    {
        if (::minTime > it.local_timestamp) ::minTime = it.local_timestamp;
        if (::maxTime < it.local_timestamp) ::maxTime = it.local_timestamp; 
        TW = maxTime - minTime; 
    }    
}

struct Instrument_Trades {
    string path_to_data; 
    uint64_t T; 
    vector<Trade> data; 
    vector<Candle> Candles_data; 
    int CandlesAmount; 
    
    uint64_t TW; 
    uint64_t MaxT;
    uint64_t MinT;

    
    Instrument_Trades(string p, uint64_t T) : path_to_data(p), T(T) {

        data = GetTradeData(path_to_data);

        MinT = UINT64_MAX;
        MaxT = 0;
        for (const auto& trade : data) {
            if (trade.local_timestamp < MinT) MinT = trade.local_timestamp;
            if (trade.local_timestamp > MaxT) MaxT = trade.local_timestamp;
        }

        TW = MaxT - MinT;
        CandlesAmount = TW / T + 1;

        for (int i = 0; i < CandlesAmount; i++) {
            Candles_data.push_back(Candle(MinT + T * i, T, data));
        }
    }
};



void GetTradeData(string instrument, vector<Trade>& v)
    {

        filesystem::path file_path = instrument;
        filesystem::directory_entry entry(file_path);
    
        ifstream data_file(entry.path()); 
        string line;

        getline(data_file, line);
 
        while (getline(data_file, line))
        {
            Trade T = parseString(line);
            v.push_back(T);    
        }
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
            Trade T = parseString(line);
            v.push_back(T);    
        }
    return v;     
}
// A - Avarage(2), C - close(1)

struct Strategy_back {
    int now = 0 ;
    vector <vector <pair<char, int> > > Actions_storage; 
    vector<pair<char, int>> data;  // <Action, Volume>
    vector <Candle> CandleData; 

    double PnL = 0.0;
    double TradedVolume = 0.0;
    double SharpeRatio = 0.0;
    double SortinoRatio = 0.0;
    double MaxDrawdown = 0.0;
    double avdHoldTime = 0.0;
    int NumOfPosFlips = 0;

    int currentPosition = 0; 
    double currentPnL = 0.0;
    vector<double> pnlHistory;
    int flips = 0;            
    int holdingTime = 0;
    int lastFlipIndex = 0;


    Strategy_back(vector<pair<char, int>> data_, vector<Candle> CandleData_) : data(data_), CandleData(CandleData_) 
    { if (data.size() != CandleData.size()) exit(2); }

    class iterator {
    public:
        Strategy_back& parent;
        int current;

        iterator(Strategy_back& p, int cr) : parent(p), current(cr) {}

        
        iterator operator++() {
            if (current >= parent.data.size()) exit(2);
            current++;
            parent.now++; 
            return *this;
        }

       
        pair<char, int>& operator*() {
            auto& action = parent.data[current];
            char actionType = action.first;
            int volume = action.second;

            
            if (actionType == 'C') {  
                parent.executeTrade(volume, parent.CandleData[parent.now].close_price, current);
                
                
                
            } else if (actionType == 'A') {  
                if (volume < 0) 
                {
                    parent.executeTrade(volume, parent.CandleData[parent.now].avg_sell_price, current);
                }  else {
                parent.executeTrade(volume, parent.CandleData[parent.now].avg_buy_price, current);
                
                }
            }

            return parent.data[current];
        }

        bool operator!=(const iterator& other) {
            return current != other.current;
        }
    };

    iterator begin() {
        return iterator(*this, 0);
    }

    iterator end() {
        return iterator(*this, data.size());
    }

    
    void executeTrade(int volume, double price, int& cur) {
    if (volume == 0) return;

    double prevPosition = currentPosition;
    currentPosition += volume;
    double tradePnL = volume * price;
    PnL += tradePnL;
    pnlHistory.push_back(PnL);

    TradedVolume += abs(volume);

    if ((prevPosition > 0 && currentPosition < 0) || (prevPosition < 0 && currentPosition > 0)) {
        flips++;
        NumOfPosFlips = flips;
        holdingTime += cur - lastFlipIndex; // Accumulate holding time
        lastFlipIndex = cur; // Update last flip index
    }
}

void calculateRatios() {
    if (pnlHistory.empty()) return;

    double meanPnL = accumulate(pnlHistory.begin(), pnlHistory.end(), 0.0) / pnlHistory.size();
    double stdDev = sqrt(accumulate(pnlHistory.begin(), pnlHistory.end(), 0.0,
        [meanPnL](double acc, double pnl) {
            return acc + (pnl - meanPnL) * (pnl - meanPnL);
        }) / pnlHistory.size());

    SharpeRatio = meanPnL / stdDev;

    double downsideDev = sqrt(accumulate(pnlHistory.begin(), pnlHistory.end(), 0.0,
        [meanPnL](double acc, double pnl) {
            return acc + ((pnl < meanPnL) ? (pnl - meanPnL) * (pnl - meanPnL) : 0);
        }) / pnlHistory.size());

    SortinoRatio = meanPnL / downsideDev;

    double peakPnL = pnlHistory[0];
    for (const double& pnl : pnlHistory) {
        if (pnl > peakPnL) peakPnL = pnl;
        MaxDrawdown = max(MaxDrawdown, peakPnL - pnl);
    }

    avdHoldTime = (flips > 0) ? static_cast<double>(holdingTime) / flips : 0.0;  // Calculate average holding time safely
}
void DisplayRes()
{
    cout << "PnL: " << this->PnL << endl;
    cout << "Traded Volume: " << this->TradedVolume << endl;
    cout << "Sharpe Ratio: " << this->SharpeRatio << endl;
    cout << "Sortino Ratio: " << this->SortinoRatio << endl;
    cout << "Max Drawdown: " << this->MaxDrawdown << endl;
    cout << "Average Holding Time: " << this->avdHoldTime << endl;
    cout << "Position Flips: " << this->NumOfPosFlips << endl;
}
};
class Strategy_for_Instrument
{
    public: 
    vector <Strategy_back> Strategies; 

    Strategy_for_Instrument(vector <Strategy_back> Strategies_) : Strategies(Strategies_) 
    {}

    void CountResultsForStrategy(int index)
    {
        if (index >= Strategies.size()) exit(3);
        for (auto& action : Strategies[index]) {} 
        Strategies[index].calculateRatios(); 
    }  
    void DisplayResForStrategy(int index)
    {
        Strategies[index].DisplayRes(); 
    }
    
}; 

class Simulator
{
    public: 
    vector <Strategy_for_Instrument> Global_data; 

    void addStrategy(Strategy_for_Instrument obj)
    {
        Global_data.push_back(obj); 
    }
    // Вывод результатов по всем стратегиям для всех инструментов
    void DisplayResultsForAllInstruments() {
        for (size_t i = 0; i < Global_data.size(); i++) {
            cout << "Instrument " << i + 1 << " results:\n";
            DisplayResultsForInstrument(i);
            cout << '\n';
        }
    }

    // Вывод результатов по одной стратегии для указанного инструмента
    void DisplayResultsForInstrument(int instrument_index) {
        if (instrument_index >= Global_data.size()) {
            cout << "Invalid instrument index.\n";
            return;
        }
        
        for (size_t i = 0; i < Global_data[instrument_index].Strategies.size(); i++) {
            cout << "Strategy " << i + 1 << " results:\n";
            Global_data[instrument_index].DisplayResForStrategy(i);
            cout << '\n';
        }
    }

    
};

int main() 
{   
    Instrument_Trades Ins(R"(C:\Users\Maxim\OneDrive\Рабочий стол\CMF\Task1\trades_dogeusdt.csv)", 201598305508); 
    for (auto t : Ins.Candles_data)
    {
        cout << '\n'; 
        t.ShowInformation();
        cout << '\n'; 
    }

    vector <pair <char, int > > info = {{'A', 10}, {'C',  -5}, {'A', 8}};
    Strategy_back S1(info, Ins.Candles_data);  
    vector <Strategy_back> v = {S1}; 
    //Strategy myStrategy(info, data_for_Strategy); 
    Strategy_for_Instrument SI1(v); 
    SI1.CountResultsForStrategy(0); 
    SI1.DisplayResForStrategy(0); 
    Simulator sim; 
    sim.addStrategy(SI1);
    sim.DisplayResultsForAllInstruments();
    return 0; 
    
}




