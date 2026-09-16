#include "DxLib.h"
#include <stdlib.h>

// 影をつけた文字列を表示する関数
void drawText(int x, int y, int col, const char* txt, int siz)
{
	SetFontSize(siz);
	DrawString(x + 3, y + 3, txt, GetColor(0, 0, 0));
	DrawString(x, y, txt, col);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	// 定数
	const int WIDTH = 920, HEIGHT = 640;	// ウィンドウの幅と高さ

	SetWindowText("刹那の一閃 ― 抜刀対決 ―");	// ウィンドウのタイトル
	SetGraphMode(WIDTH, HEIGHT, 32);		// ウィンドウの大きさとカラービット数
	ChangeWindowMode(true);					// ウィンドウモードで起動
	if (DxLib_Init() == -1) return -1;		// ライブラリ初期化
	SetBackgroundColor(20, 20, 20);			// 背景色（暗いグレー）
	SetDrawScreen(DX_SCREEN_BACK);			// 裏画面描画

	// 画像の読み込み
	int imgTitle = LoadGraph("image/title.jpg");
	int imgBg = LoadGraph("image/bg.png");
	int imgBg1 = LoadGraph("image/bg1.png");
	int imgBg2 = LoadGraph("image/bg2.png");
	int imgBg3 = LoadGraph("image/bg3.png");

	// ゲーム進行に関する変数
	enum { TITLE, RULE, WAIT, CUT, CRITICAL, RESULT };
	int scene = TITLE;
	int timer = 0;

	// 対決ロジック用の変数
	int waitTime = 0;         // 合図が出るまでのランダム待機時間
	int reactionTime = 0;     // プレイヤーの反応速度
	int enemyTime = 0;        // 敵の反応速度
	int wins = 0;             // 連勝数
	enum { WIN, LOSE1, LOSE2, FLYING };
	int resultType = WIN;

	// タイマー計測用
	int signalStartTime = 0;
	int criticalTimer = 0;

	// マウス入力トリガー管理用
	int mouseNow = 0;
	int mouseOld = 0;

	// スペース入力トリガー管理用
	int spaceNow = 0;
	int spaceOld = 0;

	// サウンドの読み込みと音量設定
	int title = LoadSoundMem("sound/Title.mp3");
	int wait = LoadSoundMem("sound/Wait.mp3");
	int win = LoadSoundMem("sound/EndWin.mp3");
	int lose = LoadSoundMem("sound/EndLose.mp3");
	int flying = LoadSoundMem("sound/Flying.mp3");
	int seNuku = LoadSoundMem("sound/Nuku.mp3");
	int seCritical = LoadSoundMem("sound/Critical.mp3");
	ChangeVolumeSoundMem(128, title);
	ChangeVolumeSoundMem(128, win);
	ChangeVolumeSoundMem(128, lose);

	PlaySoundMem(title, DX_PLAYTYPE_LOOP); // BGMをループ再生
	
	while (1)	// メインループ
	{
		ClearDrawScreen();	// 画面クリア

		// マウス入力状態の更新（トリガー判定用）
		mouseOld = mouseNow;
		mouseNow = GetMouseInput();
		// スペース入力状態の更新（トリガー判定用）
		spaceOld = spaceNow;
		spaceNow = CheckHitKey(KEY_INPUT_SPACE);

		bool isSpacePush = spaceNow && !spaceOld;
		// 今回のフレームで左クリックされたか（押し下げた瞬間のみ true）
		bool isMousePush = (mouseNow & MOUSE_INPUT_LEFT) && !(mouseOld & MOUSE_INPUT_LEFT);

		timer++; // タイマーカウント

		switch (scene)
		{
		case TITLE: // タイトル画面

			DrawExtendGraph(0, 0, 920,640, imgTitle, false);

			// UI表示（連勝数）
			SetFontSize(24);
			DrawFormatString(10, 10, GetColor(240, 210, 130), "連勝数: %d", wins);

			drawText(330, 90, GetColor(220, 180, 70), "刹那の一閃", 60);

			drawText(200, 210, GetColor(255, 255, 255), "―― 抜刀対決 ――", 60);

			drawText(350, 340, GetColor(220, 180, 70), "――――", 60);

			drawText(320, 560, GetColor(220, 180, 70), "スペースでルール説明へ", 30);

			if (timer % 60 < 30)
			{
				drawText(350, 450, GetColor(255, 255, 255), "CLICK TO START", 30);
			}

			// スペースキーでルール説明
			if (isSpacePush)
			{
				scene = RULE;
				timer = 0;
			}
			// マウスクリックでゲーム開始
			else if (isMousePush)
			{
				scene = WAIT;
				timer = 0;
				// 2秒〜5秒（約120〜300フレーム）のランダムな溜め時間
				waitTime = GetRand(180) + 120;
				StopSoundMem(title); // BGMを停止
				PlaySoundMem(wait, DX_PLAYTYPE_LOOP); // BGMをループ再生
			}
			break;

		case RULE:
			DrawExtendGraph(0, 0, 920, 640, imgBg, false);

			SetFontSize(60);
			DrawString(350, 60, "遊び方", GetColor(220, 180, 70));

			SetFontSize(30);

			DrawString(150, 160, "① 「見切ったり！」の合図を待つ", GetColor(255, 255, 255));
			DrawString(150, 220, "② 合図が出た瞬間にクリック！", GetColor(255, 255, 255));
			DrawString(150, 280, "③ 敵より早く抜刀できれば勝ち！", GetColor(255, 255, 255));

			DrawString(150, 360, "※ 合図の前にクリックするとフライング", GetColor(220, 100, 100));

			SetFontSize(25);
			DrawString(300, 500, "スペースキーでタイトルへ", GetColor(200, 180, 100));

			// タイトルへ戻る
			if (timer > 30 && isSpacePush)
			{
				scene = TITLE;
				timer = 0;
			}
			break;

		case WAIT: // 構え・合図待ち状態
			DrawExtendGraph(0, 0, 920, 640, imgBg1, false);

			SetFontSize(40);
			DrawString(360, 100, "じっと待て...", GetColor(0, 150, 255));

			// 合図が出る前にクリック（フライング判定）
			if (isMousePush)
			{
				scene = RESULT;
				timer = 0;
				resultType = FLYING;
				PlaySoundMem(flying, DX_PLAYTYPE_BACK); // BGMを再生
				StopSoundMem(wait);
			}
			// 規定時間が経過したら「見切ったり！」の合図を出す
			else if (timer >= waitTime)
			{
				scene = CUT;
				timer = 0;
				signalStartTime = GetNowCount(); // 計測開始
			}
			break;

		case CUT: // 合図発生！抜刀入力待ち

			DrawExtendGraph(0, 0, 920, 640, imgBg2, false);

			SetFontSize(80);
			DrawString(160, 180, "見切ったり！", GetColor(255, 0, 0));

			if (isMousePush)
			{
				int nowTime = GetNowCount();
				reactionTime = nowTime - signalStartTime;

				// 敵の反応時間を計算（連勝するほど敵が速くなる）
				enemyTime = (GetRand(150) + 400) - (wins * 13);
				if (enemyTime < 250) enemyTime = 250; // 敵の最速下限値

				// 勝敗判定
				if (reactionTime < enemyTime)
				{
					resultType = WIN;
					wins++;

					// 250ms以下ならクリティカル演出
					if (reactionTime <= 250)
					{
						scene = CRITICAL;
						criticalTimer = 0;
						PlaySoundMem(seCritical, DX_PLAYTYPE_BACK); // 効果音
						PlaySoundMem(win, DX_PLAYTYPE_LOOP); // BGMをループ再生
					}
					else
					{
						scene = RESULT;
						PlaySoundMem(win, DX_PLAYTYPE_LOOP); // BGMをループ再生
					}
				}
				else
				{
					resultType = LOSE1;
					wins = 0;
					scene = RESULT;
					PlaySoundMem(lose, DX_PLAYTYPE_LOOP); // BGMをループ再生
				}

				timer = 0;
				StopSoundMem(wait);
				PlaySoundMem(seNuku, DX_PLAYTYPE_BACK); // 効果音
			}
			// 一定時間クリックされなければ負け
			else if (timer >= 120)
			{
				resultType = LOSE2;
				wins = 0;
				scene = RESULT;

				enemyTime = (GetRand(150) + 400) - (wins * 13);
				if (enemyTime < 250) enemyTime = 250; // 敵の最速下限値
				
				StopSoundMem(wait);
				PlaySoundMem(seNuku, DX_PLAYTYPE_BACK); // 効果音
				PlaySoundMem(lose, DX_PLAYTYPE_LOOP); // BGMをループ再生
			}
			break;

		case CRITICAL: // クリティカル演出

			criticalTimer++;

			// 最初暗転
			if (criticalTimer < 5)
			{
				DrawBox(0, 0, 920, 640, GetColor(255, 255, 255), true);
			}
			else if (criticalTimer < 15)
			{
				DrawBox(0, 0, 920, 640, GetColor(0, 0, 0), true);
			}
			else
			{
				// 背景
				DrawBox(0, 0, 920, 640, GetColor(10, 10, 10), true);

				// 飾り
				SetFontSize(30);
				DrawString(280, 100, "――――――――――", GetColor(220, 180, 70));
				DrawString(280, 450, "――――――――――", GetColor(220, 180, 70));

				// 称号
				drawText(300, 195, GetColor(230, 190, 70), "神速", 100);

				// 反応速度
				SetFontSize(30);
				DrawFormatString(350, 330, GetColor(220, 220, 220), "%d ms", reactionTime);

				// サブタイトル
				SetFontSize(35);
				DrawString(270, 390, "―― 刹那一閃 ――", GetColor(180, 140, 50));
			}

			// 結果画面へ
			if (criticalTimer > 60)
			{
				scene = RESULT;
				timer = 0;
			}
			break;

		case RESULT: // 勝敗結果表示
			DrawExtendGraph(0, 0, 920, 640, imgBg3, false);

			SetFontSize(50);
			if (resultType == WIN)
			{
				DrawString(300, 140, "【 勝利 】", GetColor(0, 255, 0));
				DrawFormatString(170, 220, GetColor(255, 255, 255), "あなたの速度: %d ms", reactionTime);
				DrawFormatString(170, 270, GetColor(170, 170, 170), "敵の速度    : %d ms", enemyTime);
				if (reactionTime <= 250)
				{
					drawText(170, 330, GetColor(230, 190, 70), "神速", 50);
				}
				else if (reactionTime <= 300)
				{
					drawText(170, 330, GetColor(150, 80, 190), "一閃", 50);
				}
				else if (reactionTime <= 400)
				{
					drawText(170, 330, GetColor(90, 150, 190), "達人", 50);
				}
				else
				{
					drawText(170, 330, GetColor(130, 130, 130), "未熟", 50);
				}
			}
			else if (resultType == LOSE1)
			{
				DrawString(300, 140, "【 敗北 】", GetColor(100, 100, 255));
				DrawFormatString(170, 220, GetColor(255, 255, 255), "あなたの速度: %d ms", reactionTime);
				DrawFormatString(170, 270, GetColor(255, 136, 136), "敵の速度    : %d ms", enemyTime);
			}
			else if (resultType == LOSE2)
			{
				DrawString(300, 140, "【 敗北 】", GetColor(100, 100, 255));
				DrawFormatString(170, 220, GetColor(255, 255, 255), "あなたの速度: --- ms");
				DrawFormatString(170, 270, GetColor(255, 136, 136), "敵の速度    : %d ms", enemyTime);

				SetFontSize(35);
				DrawFormatString(170, 330, GetColor(255, 0, 0), "機を逸した");

			}
			else if (resultType == FLYING)
			{
				DrawString(260, 140, "【 時期尚早 】", GetColor(255, 0, 0));
				SetFontSize(30);
				DrawString(280, 230, "焦って刀を抜いてしまった...", GetColor(255, 255, 255));
				wins = 0;
			}

			if (timer % 60 < 30)
			{
				SetFontSize(24);
				DrawString(320, 500, "クリックで次の対決へ", GetColor(255, 255, 0));
			}

			// UI表示（連勝数）
			SetFontSize(24);
			DrawFormatString(10, 10, GetColor(240, 210, 130), "連勝数: %d", wins);

			// 誤連打防止のため結果画面遷移後 30フレーム経過してから入力を受け付ける
			if (timer > 30 && isMousePush)
			{
				scene = TITLE;
				timer = 0;
				StopSoundMem(win); // BGMを停止
				StopSoundMem(lose); // BGMを停止
				PlaySoundMem(title, DX_PLAYTYPE_LOOP); // BGMをループ再生
			}
			break;
		}

		ScreenFlip();	//裏画面の内容を表画面に反映させる
		WaitTimer(16);
		if (ProcessMessage() == -1) break;	//Windowsから情報を受け取りエラーが起きたら終了
		if (CheckHitKey(KEY_INPUT_ESCAPE) == 1) break;	//ESCキーが押されたら終了
	}
	DxLib_End();	// ライブラリ終了処理
	return 0;
}