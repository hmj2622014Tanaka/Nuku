#include "DxLib.h"
#include <stdlib.h>

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

	// ゲーム進行に関する変数
	enum { TITLE, WAIT, CUT, RESULT };
	int scene = TITLE;
	int timer = 0;

	// 対決ロジック用の変数
	int waitTime = 0;         // 合図が出るまでのランダム待機時間（フレーム数）
	int reactionTime = 0;     // プレイヤーの反応速度（ミリ秒）
	int enemyTime = 0;        // 敵の反応速度（ミリ秒）
	int wins = 0;             // 連勝数
	enum { WIN, LOSE, FLYING };
	int resultType = WIN;

	// タイマー計測用
	int signalStartTime = 0;

	// マウス入力トリガー管理用
	int mouseNow = 0;
	int mouseOld = 0;

	while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)	// メインループ
	{
		ClearDrawScreen();	// 画面クリア

		// マウス入力状態の更新（トリガー判定用）
		mouseOld = mouseNow;
		mouseNow = GetMouseInput();
		// 今回のフレームで左クリックされたか（押し下げた瞬間のみ true）
		bool isMousePush = (mouseNow & MOUSE_INPUT_LEFT) && !(mouseOld & MOUSE_INPUT_LEFT);

		timer++; // タイマーカウント

		// 背景の描画（シーン共通）
		if (imgBg != -1) DrawGraph(0, 0, imgBg, false);

		switch (scene)
		{
		case TITLE: // タイトル画面
			DrawExtendGraph(0, 0, 920,640, imgTitle, false);

			SetFontSize(60);

			// 影
			DrawString(333, 93, "刹那の一閃", GetColor(0, 0, 0));

			// 本体
			DrawString(330, 90, "刹那の一閃", GetColor(220, 180, 70));

			// 影
			DrawString(203, 213, "―― 抜刀対決 ――", GetColor(0, 0, 0));

			// 本体
			DrawString(200, 210, "―― 抜刀対決 ――", GetColor(255, 255, 255));

			// 影
			DrawString(353, 343, "――――", GetColor(0, 0, 0));

			// 本体
			DrawString(350, 340, "――――", GetColor(220, 180, 70));

			if (timer % 60 < 30)
			{
				SetFontSize(30);
				// 影
				DrawString(353, 453, "CLICK TO START", GetColor(0, 0, 0));

				// 本体
				DrawString(350, 450, "CLICK TO START", GetColor(255, 255, 255));
			}

			if (isMousePush)
			{
				scene = WAIT;
				timer = 0;
				// 2秒〜5秒（約120〜300フレーム）のランダムな溜め時間
				waitTime = GetRand(180) + 120;
			}
			break;

		case WAIT: // 構え・合図待ち状態
			DrawExtendGraph(0, 0, 920, 640, imgBg, false);

			SetFontSize(40);
			DrawString(360, 100, "じっと待て...", GetColor(0, 150, 255));

			// 合図が出る前にクリック（フライング判定）
			if (isMousePush)
			{
				scene = RESULT;
				timer = 0;
				resultType = FLYING;
			}
			// 規定時間が経過したら「見切ったり！」の合図を出す
			else if (timer >= waitTime)
			{
				scene = CUT;
				timer = 0;
				signalStartTime = GetNowCount(); // 計測開始（ミリ秒）
			}
			break;

		case CUT: // 合図発生！抜刀入力待ち
			DrawExtendGraph(0, 0, 920, 640, imgBg1, false);

			SetFontSize(80);
			DrawString(160, 180, "見切ったり！", GetColor(255, 0, 0));

			if (isMousePush)
			{
				int nowTime = GetNowCount();
				reactionTime = nowTime - signalStartTime;

				// 敵の反応時間を計算（連勝するほど敵が速くなる）
				enemyTime = (GetRand(150) + 400) - (wins * 15);
				if (enemyTime < 220) enemyTime = 220; // 敵の最速下限値

				// 勝敗判定
				if (reactionTime < enemyTime)
				{
					resultType = WIN;
					wins++;
				}
				else
				{
					resultType = LOSE;
					wins = 0;
				}

				scene = RESULT;
				timer = 0;
			}
			break;

		case RESULT: // 勝敗結果表示
			DrawExtendGraph(0, 0, 920, 640, imgBg2, false);

			SetFontSize(50);
			if (resultType == WIN)
			{
				DrawString(300, 140, "【 勝利 】", GetColor(0, 255, 0));
				DrawFormatString(170, 220, GetColor(255, 255, 255), "あなたの速度: %d ms", reactionTime);
				DrawFormatString(170, 270, GetColor(170, 170, 170), "敵の速度    : %d ms", enemyTime);
				if (reactionTime < 200)
				{
					// 影
					DrawString(172, 332, "神速", GetColor(40, 40, 40));

					// 本体
					DrawString(170, 330, "神速", GetColor(230, 190, 70));
				}
				else if (reactionTime < 300)
				{
					// 影
					DrawString(172, 332, "一閃", GetColor(40, 40, 40));

					// 本体
					DrawString(170, 330, "一閃", GetColor(150, 80, 190));
				}
				else if (reactionTime < 400)
				{
					// 影
					DrawString(172, 332, "達人", GetColor(40, 40, 40));

					// 本体
					DrawString(170, 330, "達人", GetColor(90, 150, 190));
				}
				else
				{
					// 影
					DrawString(172, 332, "未熟", GetColor(40, 40, 40));
					
					// 本体
					DrawString(170, 330, "未熟", GetColor(130, 130, 130));
				}
			}
			else if (resultType == LOSE)
			{
				DrawString(300, 140, "【 敗北 】", GetColor(100, 100, 255));
				DrawFormatString(170, 220, GetColor(255, 255, 255), "あなたの速度: %d ms", reactionTime);
				DrawFormatString(170, 270, GetColor(255, 136, 136), "敵の速度    : %d ms", enemyTime);
			}
			else if (resultType == FLYING)
			{
				DrawString(260, 140, "【 フライング 】", GetColor(255, 0, 0));
				SetFontSize(30);
				DrawString(280, 230, "焦って刀を抜いてしまった...", GetColor(255, 255, 255));
				wins = 0;
			}

			if (timer % 60 < 30)
			{
				SetFontSize(24);
				DrawString(320, 500, "クリックで次の対決へ", GetColor(255, 255, 0));
			}

			// 誤連打防止のため結果画面遷移後 30フレーム経過してから入力を受け付ける
			if (timer > 30 && isMousePush)
			{
				scene = TITLE;
				timer = 0;
			}
			break;
		}

		// UI表示（連勝数）
		SetFontSize(24);
		DrawFormatString(10, 10, GetColor(255, 255, 0), "連勝数: %d", wins);

		ScreenFlip();	//裏画面の内容を表画面に反映させる
		WaitTimer(16);	//一定時間待つ
		if (ProcessMessage() == -1) break;	//Windowsから情報を受け取りエラーが起きたら終了
		if (CheckHitKey(KEY_INPUT_ESCAPE) == 1) break;	//ESCキーが押されたら終了
	}

	DxLib_End();	// ライブラリ終了処理
	return 0;
}