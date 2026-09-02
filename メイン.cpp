#include "DxLib.h"
#include <stdlib.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	// 定数
	const int WIDTH = 720, HEIGHT = 640;	// ウィンドウの幅と高さ

	SetWindowText("一撃必殺！抜刀対決");	// ウィンドウのタイトル
	SetGraphMode(WIDTH, HEIGHT, 32);		// ウィンドウの大きさとカラービット数
	ChangeWindowMode(true);					// ウィンドウモードで起動
	if (DxLib_Init() == -1) return -1;		// ライブラリ初期化
	SetBackgroundColor(20, 20, 20);			// 背景色（暗いグレー）
	SetDrawScreen(DX_SCREEN_BACK);			// 裏画面描画

	// 画像の読み込み
	int imgTitle = LoadGraph("image/title.png");
	int imgBg = LoadGraph("image/bg.png");
	int imgPlayer = LoadGraph("image/player.png");
	int imgEnemy = LoadGraph("image/enemy.png");

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

		// --------------------------------------------------
		// 背景・キャラクターの描画（シーン共通）
		// --------------------------------------------------
		if (imgBg != -1) DrawGraph(0, 0, imgBg, false);

		// キャラクター描画
		if (imgPlayer != -1) DrawGraph(180, 320, imgPlayer, true);
		else DrawBox(180, 320, 260, 480, GetColor(0, 255, 255), true); // プレイヤー（青）

		if (imgEnemy != -1) DrawGraph(460, 320, imgEnemy, true);
		else DrawBox(460, 320, 540, 480, GetColor(255, 100, 100), true); // 敵（赤）

		switch (scene)
		{
		case TITLE: // タイトル画面
			SetFontSize(60);
			DrawString(160, 160, "一撃必殺！抜刀対決", GetColor(255, 255, 255));

			if (timer % 60 < 30)
			{
				SetFontSize(30);
				DrawString(200, 400, "クリック で対決開始", GetColor(0, 255, 0));
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
			SetFontSize(40);
			DrawString(260, 100, "じっと待て...", GetColor(204, 204, 204));

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
			SetFontSize(80);
			DrawString(140, 180, "見切ったり！", GetColor(255, 0, 0));

			if (isMousePush)
			{
				int nowTime = GetNowCount();
				reactionTime = nowTime - signalStartTime;

				// 敵の反応時間を計算（連勝するほど敵が速くなる）
				enemyTime = (GetRand(100) + 250) - (wins * 15);
				if (enemyTime < 120) enemyTime = 120; // 敵の最速下限値

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
			SetFontSize(50);
			if (resultType == WIN)
			{
				DrawString(260, 140, "【 勝利 】", GetColor(0, 255, 255));
				DrawFormatString(180, 220, GetColor(255, 255, 255), "あなたの速度: %d ms", reactionTime);
				DrawFormatString(180, 270, GetColor(170, 170, 170), "敵の速度    : %d ms", enemyTime);
			}
			else if (resultType == LOSE)
			{
				DrawString(260, 140, "【 敗北 】", GetColor(136, 136, 136));
				DrawFormatString(180, 220, GetColor(255, 255, 255), "あなたの速度: %d ms", reactionTime);
				DrawFormatString(180, 270, GetColor(255, 136, 136), "敵の速度    : %d ms", enemyTime);
			}
			else if (resultType == FLYING)
			{
				DrawString(200, 140, "【 フライング 】", GetColor(255, 0, 0));
				SetFontSize(30);
				DrawString(180, 230, "焦って刀を抜いてしまった...", GetColor(255, 255, 255));
				wins = 0;
			}

			if (timer % 60 < 30)
			{
				SetFontSize(24);
				DrawString(220, 500, "クリックで次の対決へ", GetColor(255, 255, 0));
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

		ScreenFlip(); // 裏画面を表画面に反映
	}

	DxLib_End();	// ライブラリ終了処理
	return 0;
}