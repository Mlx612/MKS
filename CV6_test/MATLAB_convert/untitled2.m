%% build_ntc_lookup.m  (по шагам из задания)
clear; clc; close all;

% --- Konstants pro dělič a ADC ---
Rfix = 10000;      % 10 kΩ
FS   = 1023;       % 10bit ADC (2^10 - 1)

% --- 1) Načtení tabulky [Teplota °C, R_NTC Ω] ---
% Soubor ntc.csv je "připraven pro csvread" (bez hlavičky).
M = csvread('ntc.csv');   % [T_degC, R_ohm]
t = M(:,1);               % teplota [°C]
R = M(:,2);               % odpor NTC [Ω]

% --- 2) Přepočet odporu -> kód ADC ---
% Vadc/Vref = Rntc/(Rntc + Rfix);  ad = round(FS * Vadc/Vref);
% ad = FS * (R ./ (R + Rfix));



% ВАРИАНТ 2: работаем в Омах
R   = R * 1000;         % раскомментируй, если в CSV были kΩ
Rfix= 10000;
ad  = round( 1023 * ( R ./ (R + Rfix) ) ); % или зеркальная формула

% --- 3) Polynomiální aproximace t(ad) ---
p = polyfit(ad, t, 15);   % stupeň 10, jak v zadání

% --- 4) Vytvoření tabulky pro všechny kódy ADC 0..1023 ---
ad2 = 0:FS;
t2  = round( polyval(p, ad2), 1 );   % zaokrouhlení //na 0.1 °C

% --- 5) Graf pro kontrolu (jak v zadání) ---
figure;
plot(ad, t, 'bo');          % původní body (ad, t)
hold on;
plot(ad2, t2, 'r');         % interpolovaná křivka (ad2, t2)
grid on;
xlabel('ADC value');
ylabel('Temperature [°C]');
title('NTC: původní data a polynomiální aproximace');
% legend('Body z tabulky','polyfit -> polyval','Location','best');

% --- 6) Uložení lookup tabulky do souboru pro C ---
% "hodnoty oddělené čárkou, v desetinných stupních"
dlmwrite('data.dlm', t2, 'delimiter', ',', 'precision', '%.1f');